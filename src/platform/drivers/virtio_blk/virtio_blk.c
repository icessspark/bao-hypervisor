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
#include <at_utils.h>
#include <drivers/virtio.h>
#include <drivers/virtio_blk.h>
#include <drivers/virtio_mmio.h>
#include <drivers/virtio_ring.h>

#include <cpu.h>
#include <interrupts.h>
#include <mem.h>
#include <printf.h>
#include <string.h>
#include <vm.h>

#define VIRTIO_MMIO_BASE 0x0a003000
#define QUEUE_SIZE 8
static void *base = NULL;

static struct {
    char pages[2 * PAGE_SIZE];
    struct vring_desc *desc;
    u16 *avail;
    struct vring_used *used;
} __attribute__((aligned(PAGE_SIZE))) disk;

static char status = 0;

static spinlock_t virtio_lock;

// static char *debug_buf = NULL;

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

void virtio_blk_handler()
{
    // printf("disk.used->ring->id %x\n", disk.used->ring->id);
    // printf("disk.used->idx %x\n", disk.used->idx);
    // printf("status %x\n", status);

    // for (int i = 0; i < 4096; ++i) {
    //     printf("%02x ", debug_buf[i]);
    //     if ((i + 1) % 32 == 0) {
    //         printf("\n");
    //     }
    // }
    if (status != 0) {
        printf("status %d\n", status);
        WARNING("virtio_blk_handler: status != 0");
    }
    writel(1, base + VIRTIO_MMIO_INTERRUPT_ACK);
    // spin_unlock(&virtio_lock);
    interrupts_vm_inject(cpu.vcpu->vm, 0x10 + 32, 0);
}

void virtio_disk_rw(u64 sector, u64 count, char *buf, int write)
{
    struct virtio_blk_outhdr outhdr;

    if (write)
        outhdr.type = VIRTIO_BLK_T_OUT;  // write the disk
    else
        outhdr.type = VIRTIO_BLK_T_IN;  // read the disk
    outhdr.ioprio = 0;
    outhdr.sector = sector;

    disk.desc[0].addr = (u64)pa(&outhdr);
    disk.desc[0].len = sizeof(outhdr);
    disk.desc[0].flags = VRING_DESC_F_NEXT;
    disk.desc[0].next = 1;

    disk.desc[1].addr = (u64)pa(buf);
    disk.desc[1].len = 512 * count;
    if (write)
        disk.desc[1].flags = 0;  // device reads b->data
    else
        disk.desc[1].flags = VRING_DESC_F_WRITE;  // device writes b->data
    disk.desc[1].flags |= VRING_DESC_F_NEXT;
    disk.desc[1].next = 2;

    status = 0;
    disk.desc[2].addr = (u64)pa(&status);
    disk.desc[2].len = 1;
    disk.desc[2].flags = VRING_DESC_F_WRITE;  // device writes the status
    disk.desc[2].next = 0;

    // avail[0] is flags
    // avail[1] tells the device how far to look in avail[2...].
    // avail[2...] are desc[] indices the device should process.
    // we only tell device the first index in our chain of descriptors.
    disk.avail[2 + (disk.avail[1] % QUEUE_SIZE)] = 0;
    asm("dsb sy");
    disk.avail[1] = disk.avail[1] + 1;

    writel(0, base + VIRTIO_MMIO_QUEUE_NOTIFY);
    int i = 0;
    while (readl(base + VIRTIO_MMIO_QUEUE_NOTIFY) != 0) {
        i++;
        if (i > 0x100) break;
    }
}

void virtio_blk_read(unsigned long sector, unsigned long count, void *buf)
{
    spin_lock(&virtio_lock);
    // debug_buf = buf;
    virtio_disk_rw(sector, count, buf, 0);
    spin_unlock(&virtio_lock);
}

void virtio_blk_write(unsigned long sector, unsigned long count, void *buf)
{
    spin_lock(&virtio_lock);
    // debug_buf = buf;
    virtio_disk_rw(sector, count, buf, 1);
    spin_unlock(&virtio_lock);
}
