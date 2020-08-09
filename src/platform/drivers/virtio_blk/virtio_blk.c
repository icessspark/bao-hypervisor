//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
// qemu presents a "legacy" virtio interface.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device
// virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//

// Note: adapted from
// https://github.com/mit-pdos/xv6-riscv/blob/riscv/kernel/virtio_disk.c
#include <drivers/virtio.h>
#include <drivers/virtio_blk.h>
#include <drivers/virtio_mmio.h>
#include <drivers/virtio_ring.h>
#include <at_utils.h>

#include <mem.h>
#include <cpu.h>
#include <printf.h>
#include <string.h>

#define VIRTIO_MMIO_BASE 0x0a003000
#define QUEUE_SIZE 8
static void *base = NULL;

struct buf {
    char *data;
    u8 disk;
};

static struct {
    char pages[2 * PAGE_SIZE];
    struct vring_desc *desc;
    u16 *avail;
    struct vring_used *used;

    char free[QUEUE_SIZE];
    u16 used_idx;

    struct {
        struct buf *b;
        char status;
    } info[QUEUE_SIZE];

} __attribute__((aligned(PAGE_SIZE))) disk;

static struct buf b;

static spinlock_t virtio_lock;

static inline void *pa(void *va)
{
    return (void *)el2_va2pa((uint64_t)va);
}

static void virtio_mmio_setup_vq(u32 index)
{
    writel(index, base + VIRTIO_MMIO_QUEUE_SEL);
    if (readl(base + VIRTIO_MMIO_QUEUE_PFN)) {
        panic("queue already set up");
    }
    u32 num = readl(base + VIRTIO_MMIO_QUEUE_NUM_MAX);
    if (num == 0) {
        panic("queue num max is zero");
    }
    if (num < QUEUE_SIZE) {
        panic("queue size not supported");
    }
    writel(QUEUE_SIZE, base + VIRTIO_MMIO_QUEUE_NUM);
    memset(disk.pages, 0, 2 * PAGE_SIZE);
    writel(((u64)pa(disk.pages)) >> PAGE_SHIFT, base + VIRTIO_MMIO_QUEUE_PFN);

    disk.desc = (struct vring_desc *)disk.pages;
    disk.avail =
        (u16 *)(((char *)disk.desc) + QUEUE_SIZE * sizeof(struct vring_desc));
    disk.used = (struct vring_used *)(disk.pages + PAGE_SIZE);

    for (int i = 0; i < QUEUE_SIZE; ++i) {
        disk.free[i] = 1;
    }
}

void virtio_blk_init()
{
    void *va = mem_alloc_vpage(&cpu.as, SEC_HYP_DEVICE, NULL, 1);
    mem_map_dev(&cpu.as, va, VIRTIO_MMIO_BASE, 1);
    /* Note: qemu use the last one */
    base = va + 7 * 0x200;

    if (readl(base + VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
        readl(base + VIRTIO_MMIO_VERSION) != 1 ||
        readl(base + VIRTIO_MMIO_DEVICE_ID) != 2 ||
        readl(base + VIRTIO_MMIO_VENDOR_ID) != 0x554d4551) {
        panic("could not find virtio disk");
    }

    u32 status = 0;
    status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
    writel(status, base + VIRTIO_MMIO_STATUS);

    status |= VIRTIO_CONFIG_S_DRIVER;
    writel(status, base + VIRTIO_MMIO_STATUS);

    u64 features = readl(base + VIRTIO_MMIO_DEVICE_FEATURES);
    features &= ~(1 << VIRTIO_BLK_F_RO);
    features &= ~(1 << VIRTIO_BLK_F_SCSI);
    features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
    features &= ~(1 << VIRTIO_BLK_F_MQ);
    features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
    features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
    features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
    writel((u32)features, base + VIRTIO_MMIO_DRIVER_FEATURES);

    status |= VIRTIO_CONFIG_S_FEATURES_OK;
    writel(status, base + VIRTIO_MMIO_STATUS);

    status |= VIRTIO_CONFIG_S_DRIVER_OK;
    writel(status, base + VIRTIO_MMIO_STATUS);

    writel(PAGE_SIZE, base + VIRTIO_MMIO_GUEST_PAGE_SIZE);
    virtio_mmio_setup_vq(0);
}

// find a free descriptor, mark it non-free, return its index.
static int alloc_desc()
{
    for (int i = 0; i < QUEUE_SIZE; i++) {
        if (disk.free[i]) {
            disk.free[i] = 0;
            return i;
        }
    }
    return -1;
}

// mark a descriptor as free.
static void free_desc(int i)
{
    if (i >= QUEUE_SIZE) panic("virtio_disk_intr 1");
    if (disk.free[i]) panic("virtio_disk_intr 2");
    disk.desc[i].addr = 0;
    disk.free[i] = 1;
    // wakeup(&disk.free[0]);
}

// free a chain of descriptors.
static void free_chain(int i)
{
    while (1) {
        free_desc(i);
        if (disk.desc[i].flags & VRING_DESC_F_NEXT)
            i = disk.desc[i].next;
        else
            break;
    }
}

static int alloc3_desc(int *idx)
{
    for (int i = 0; i < 3; i++) {
        idx[i] = alloc_desc();
        if (idx[i] < 0) {
            for (int j = 0; j < i; j++) free_desc(idx[j]);
            return -1;
        }
    }
    return 0;
}

void virtio_blk_handler()
{
    while ((disk.used_idx % QUEUE_SIZE) != (disk.used->idx % QUEUE_SIZE)) {
        int id = disk.used->ring[disk.used_idx].id;

        printf("[virtio_blk_handler] id: %d, status: %d\n", id, disk.info[id].status);
        if (disk.info[id].status != 0)
            panic("[PANIC] virtio_blk_handler status\n");

        disk.info[id].b->disk = 0;  // disk is done with buf
        disk.info[id].b = 0;
        free_chain(id);
        disk.used_idx = (disk.used_idx + 1) % QUEUE_SIZE;
    }
    writel(1, base + VIRTIO_MMIO_INTERRUPT_ACK);
    spin_unlock(&virtio_lock);
}

void virtio_disk_rw(u64 sector, u64 count, struct buf *b, int write)
{
    // the spec says that legacy block operations use three
    // descriptors: one for type/reserved/sector, one for
    // the data, one for a 1-byte status result.

    // allocate the three descriptors.
    int idx[3];
    while (1) {
        if (alloc3_desc(idx) == 0) {
            break;
        }
    }

    // format the three descriptors.
    // qemu's virtio-blk.c reads them.

    struct virtio_blk_outhdr buf0;

    if (write)
        buf0.type = VIRTIO_BLK_T_OUT;  // write the disk
    else
        buf0.type = VIRTIO_BLK_T_IN;  // read the disk
    buf0.ioprio = 0;
    buf0.sector = sector;

    disk.desc[idx[0]].addr = (u64)pa(&buf0);
    disk.desc[idx[0]].len = sizeof(buf0);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (u64)pa(b->data);
    disk.desc[idx[1]].len = 512 * count;
    if (write)
        disk.desc[idx[1]].flags = 0;  // device reads b->data
    else
        disk.desc[idx[1]].flags = VRING_DESC_F_WRITE;  // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0;
    disk.desc[idx[2]].addr = (u64)pa(&disk.info[idx[0]].status);
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE;  // device writes the status
    disk.desc[idx[2]].next = 0;

    b->disk = 1;
    disk.info[idx[0]].b = b;

    // avail[0] is flags
    // avail[1] tells the device how far to look in avail[2...].
    // avail[2...] are desc[] indices the device should process.
    // we only tell device the first index in our chain of descriptors.
    disk.avail[2 + (disk.avail[1] % QUEUE_SIZE)] = idx[0];
    //__sync_synchronize();
    disk.avail[1] = disk.avail[1] + 1;

    writel(0, base + VIRTIO_MMIO_QUEUE_NOTIFY);
}

void virtio_blk_read(unsigned long sector, unsigned long count, void *buf)
{
    spin_lock(&virtio_lock);
    b.disk = 0;
    b.data = buf;
    virtio_disk_rw(sector, count, &b, 0);
}

void virtio_blk_write(unsigned long sector, unsigned long count,
                      const void *buf)
{
    spin_lock(&virtio_lock);
    b.disk = 0;
    b.data = (void *)buf;
    virtio_disk_rw(sector, count, &b, 1);
}
