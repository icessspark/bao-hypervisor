#include <at_utils.h>
#include <printf.h>
#include <virtq.h>

// FIXME: uart-pl011 9000000.pl011: no DMA platform data
// FIXME: systemd[1]: Set hostname to <kernel-4_19>
//  两个核心饥饿、死锁？

// For debug
static volatile int header_idx = 0;
static volatile int data_idx = 0;
static volatile int status_idx = 0;

static inline int next_desc(struct vring_desc *desc)
{
    return (!(desc->flags & VIRTQ_DESC_F_NEXT)) ? -1 : desc->next;
}

static uint16_t get_virt_desc_header(virtq_t *vq, struct virtio_blk_req *req,
                                     uint16_t desc_idx,
                                     struct vring_desc *desc_table)
{
    // header
    header_idx = desc_idx;  // for print
    vq->desc = &desc_table[desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;

    if (desc_flags != 0x1) {
        ERROR("desc_header flag error!");
    }

    void *vreq = ipa2va(desc_addr);
    req->type = *(uint32_t *)vreq;
    req->sector = *(uint64_t *)(vreq + 8);

    return desc_next;
}

static uint16_t get_virt_desc_data(virtq_t *vq, struct virtio_blk_req *req,
                                   uint16_t desc_idx,
                                   struct vring_desc *desc_table)
{
    // data
    data_idx = desc_idx;  // for print
    vq->desc = &desc_table[desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;
    if (desc_flags != 0x1 && desc_flags != 0x3) {
        ERROR("desc_data flag error!");
    }

    req->data_bg = desc_addr;
    req->len = desc_len;

    return desc_next;
}

static uint16_t update_virt_desc_status(virtq_t *vq, struct virtio_blk_req *req,
                                        uint16_t desc_idx,
                                        struct vring_desc *desc_table,
                                        uint32_t status)
{
    // status
    status_idx = desc_idx;  // for print
    vq->desc = &desc_table[desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;

    if (desc_flags != 0x2) {
        ERROR("desc_status flag error!");
    }

    req->status = ipa2va(desc_addr);
    *((char *)(req->status)) = (char)status;

    return desc_next;
}

static uint16_t get_avail_desc(virtq_t *vq, uint64_t avail_addr, uint32_t num)
{
    void *avail = ipa2va(avail_addr);
    vq->avail = avail;
    uint16_t avail_desc_idx = 0;

    // printk("avail_flags 0x%x idx 0x%x ring_addr 0x%x%x\n", vq->avail->flags,
    //        vq->avail->idx, u64_high_to_u32(vq->avail->ring),
    //        u64_low_to_u32(vq->avail->ring));
    // ISB();

    // printf("[*avail_ring*]\nlast_avail_idx %d cur_avail_idx %d\n",
    //        vq->last_avail_idx, vq->avail->idx);

    // virtq_disable_notify(vq);

    if (vq->avail->flags != 0) {
        WARNING("vq->avail->flags != 0");
    }

    if (!virtq_has_descs(vq)) {
        // INFO("last_avail_idx >= cur_idx!");
        return num;
    }

    // virtq_enable_notify(vq);

    if (vq->avail->idx == 0) {
        WARNING("cur_avail_idx == 0 !");
        avail_desc_idx = vq->avail->ring[num - 1];
        vq->last_avail_idx++;
        return avail_desc_idx;
    }

    for (int i = 0; i < num; i++) {
        if (i == (vq->last_avail_idx) % num) {
            avail_desc_idx = vq->avail->ring[i];
            break;
        }
    }

    vq->last_avail_idx++;

    return avail_desc_idx;
}

static inline void update_used_flags(virtq_t *vq, uint64_t used_addr)
{
    vq->used = ipa2va(used_addr);
    vq->used->flags = vq->used_flags;
    // printk("update_used_flags %d\n", vq->used->flags);
}

static void update_used_ring(virtq_t *vq, struct virtio_blk_req *req,
                             uint64_t used_addr, uint16_t desc_chain_head_idx,
                             uint32_t num)
{
    // update used ring
    update_used_flags(vq, used_addr);

    // FIXME: real write len
    vq->used->ring[vq->used->idx % num].id = desc_chain_head_idx;
    vq->used->ring[vq->used->idx % num].len = req->len;
    vq->last_used_idx = vq->used->idx;
    vq->used->idx++;
}

static void show_avail_info(virtq_t *vq, uint32_t num)
{
    for (int i = 0; i < num; i++) {
        if (i == (vq->avail->idx - 1) % num) {
            printk("* ");
        }
        printk("index %d 0x%x\n", i, vq->avail->ring[i]);
    }
}

static void show_desc_info(struct virtio_blk_req *req,
                           struct vring_desc *desc_table, uint32_t num)
{
    printk("[*desc_ring*]\n");

    for (int i = 0; i < num; i++) {
        if (i == header_idx) {
            printk("[header] \n");
        } else if (i == data_idx) {
            printk("[data] \n");
        } else if (i == status_idx) {
            printk("[status] \n");
        }
        printk("index %d desc_addr 0x%x%x len 0x%x flags 0x%x next 0x%x\n", i,
               u64_high_to_u32(desc_table[i].addr),
               u64_low_to_u32(desc_table[i].addr), desc_table[i].len,
               desc_table[i].flags, desc_table[i].next);
        if (i == header_idx) {
            printk("    req_type 0x%x req_sector 0x%x%x\n", req->type,
                   u64_high_to_u32(req->sector), u64_low_to_u32(req->sector));
        } else if (i == data_idx) {
            printk("    req_write_addr 0x%x%x req_write_len 0x%x\n",
                   u64_high_to_u32(req->data_bg), u64_low_to_u32(req->data_bg),
                   req->len);
        } else if (i == status_idx) {
            printk("    req_status_addr 0x%x%x\n", u64_high_to_u32(req->status),
                   u64_low_to_u32(req->status));
        }
    }
}

static void show_used_info(virtq_t *vq, uint32_t num)
{
    printk("[*used_ring*]\n");

    for (int i = 0; i < num; i++) {
        if (i == (vq->used->idx - 1) % num) printk("* ");
        printk("index %d id 0x%x len 0x%x\n", i, vq->used->ring[i].id,
               vq->used->ring[i].len);
    }

    printf("cur_used_idx %d next_used_idx %d\n", vq->last_used_idx,
           vq->used->idx);
}

bool process_guest_blk_notify(virtq_t *vq, virtio_mmio_t *v_m)
{
    uint64_t desc_table_addr = u32_to_u64_high(v_m->regs.q_desc_h) |
                               u32_to_u64_low(v_m->regs.q_desc_l);
    uint64_t avail_addr =
        u32_to_u64_high(v_m->regs.q_drv_h) | u32_to_u64_low(v_m->regs.q_drv_l);
    uint64_t used_addr =
        u32_to_u64_high(v_m->regs.q_dev_h) | u32_to_u64_low(v_m->regs.q_dev_l);

    struct vring_desc *desc_table = ipa2va(desc_table_addr);
    struct virtio_blk_req *req = v_m->dev->req;
    uint32_t num = vq->num;
    uint16_t avail_desc_idx = 0;
    uint16_t desc_chain_head_idx = 0;

    int process_count = 0;

    while ((avail_desc_idx = get_avail_desc(vq, avail_addr, num)) >= 0) {
        if (avail_desc_idx == num) {
            // WARNING("Unable to get desc_chain!");
            break;
        }

        // printk("avail_desc_idx %d\n", avail_desc_idx);
        // show_avail_info(vq, num);

        desc_chain_head_idx = avail_desc_idx;
        uint16_t desc_next = 0;
        // TODO: Check next_desc
        desc_next = get_virt_desc_header(vq, req, avail_desc_idx, desc_table);
        desc_next = get_virt_desc_data(vq, req, desc_next, desc_table);
        update_virt_desc_status(vq, req, desc_next, desc_table,
                                VIRTIO_BLK_S_OK);
        // show_desc_info(req, desc_table, num);

        update_used_ring(vq, req, used_addr, desc_chain_head_idx, num);
        // show_used_info(vq, num);

        // handle request
        void *buf = ipa2va(req->data_bg);
        // FIXME: decide on how to realize handler
        if (req->type > 1) {
            update_virt_desc_status(vq, req, desc_next, desc_table,
                                    VIRTIO_BLK_S_UNSUPP);
        }

        blk_req_handler(req, buf);
        process_count ++;
        // v_m->dev->handler(req, buf);
    }
    if (process_count != 0) {
        interrupts_vm_inject(cpu.vcpu->vm, 0x10 + 32, 0);
    }
    return true;
}
