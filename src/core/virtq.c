#include <at_utils.h>
#include <printf.h>
#include <virtq.h>

#define DEBUG_ON 1

static inline int next_desc(struct vring_desc *desc)
{
    return (!(desc->flags & VIRTQ_DESC_F_NEXT)) ? -1 : desc->next;
}

uint16_t get_avail_desc(virtq_t *vq, uint64_t avail_addr, uint32_t num)
{
    void *avail = ipa2va(avail_addr);
    vq->avail = avail;
    uint16_t avail_desc_idx = 0;

    // printk("avail_ipa 0x%x%x\n", u64_high_to_u32(avail_addr),
    // u64_low_to_u32(avail_addr)); printk("avail_pa 0x%x%x\n",
    // u64_high_to_u32(ipa2pa(avail_addr)), u64_low_to_u32(ipa2pa(avail_addr)));
    // printk("avail_el2_va 0x%x%x\n", u64_high_to_u32((uint64_t)avail),
    // u64_low_to_u32((uint64_t)avail));
    //    printk("[*avail_ring*]\n");

    //    printk("avail_flags 0x%x idx 0x%x ring_addr 0x%x%x\n",
    //    vq->avail->flags, vq->avail->idx,
    //        u64_high_to_u32(vq->avail->ring),
    //        u64_low_to_u32(vq->avail->ring));

    //    printf("last_avail_idx %d cur_avail_idx %d\n", vq->last_avail_idx,
    //    vq->avail->idx);
    if (vq->avail->flags != 0) {
        ERROR("vq->avail->flags != 0");
    }
    if (vq->avail->idx <= vq->last_avail_idx) {
        WARNING("available idx is not changed\n");
        return num + 1;
    }

    vq->last_avail_idx = vq->avail->idx;

    if (vq->avail->idx == 0) {
        avail_desc_idx = vq->avail->ring[num - 1];
        vq->last_avail_idx = vq->avail->idx;
        return avail_desc_idx;
    }

    for (int i = 0; i < num; i++) {
        if (i == (vq->avail->idx - 1) % num) {
            //            printk("* ");
            avail_desc_idx = vq->avail->ring[i];
        }
        //        printk("index %d 0x%x\n", i, vq->avail->ring[i]);
    }

    return avail_desc_idx;
}

void process_guest_blk_notify(virtq_t *vq, virtio_mmio_t *v_m)
{
    uint64_t desc_root_addr = u32_to_u64_high(v_m->regs.q_desc_h) +
                              u32_to_u64_low(v_m->regs.q_desc_l);
    uint64_t avail_addr =
        u32_to_u64_high(v_m->regs.q_drv_h) + u32_to_u64_low(v_m->regs.q_drv_l);
    uint64_t used_addr =
        u32_to_u64_high(v_m->regs.q_dev_h) + u32_to_u64_low(v_m->regs.q_dev_l);
    uint32_t num = vq->num;

    uint16_t avail_desc_idx = get_avail_desc(vq, avail_addr, num);
    if (avail_desc_idx >= num) {
        WARNING("unable to get desc_chain\n");
        return;
    }
    uint16_t desc_chain_head_idx = avail_desc_idx;

    // printk("desc_root_ipa 0x%x%x\n", u64_high_to_u32(desc_root_addr),
    // u64_low_to_u32(desc_root_addr)); printk("desc_root_el2_pa 0x%x%x\n",
    // u64_high_to_u32(ipa2pa(desc_root_addr)),
    // u64_low_to_u32(ipa2pa(desc_root_addr)));

    struct vring_desc *desc_root = ipa2va(desc_root_addr);

    //    printk("[*desc_ring*]\n");

    // header
    int header_idx = avail_desc_idx;  // for print
    vq->desc = &desc_root[avail_desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;

    struct virtio_blk_req *req = v_m->dev->req;
    void *vreq = ipa2va(desc_addr);

    req->type = *(uint32_t *)vreq;
    req->sector = *(uint64_t *)(vreq + 8);

    // DATA
    avail_desc_idx = desc_next;
    int data_idx = avail_desc_idx;  // for print
    vq->desc = &desc_root[avail_desc_idx];
    desc_addr = vq->desc->addr;
    desc_len = vq->desc->len;
    desc_flags = vq->desc->flags;
    desc_next = vq->desc->next;

    req->data_bg = desc_addr;
    req->len = desc_len;

    // status
    avail_desc_idx = desc_next;
    int status_idx = avail_desc_idx;  // for print
    vq->desc = &desc_root[avail_desc_idx];
    desc_addr = vq->desc->addr;
    desc_len = vq->desc->len;
    desc_flags = vq->desc->flags;
    desc_next = vq->desc->next;

    req->status_addr = desc_addr;

    void *state = ipa2va(req->status_addr);
    *(uint32_t *)state = 0;

    for (int i = 0; i < num; i++) {
        //        if(i == header_idx) {
        //            printk("[header] ");
        //        }
        //        else if(i == data_idx) {
        //            printk("[data] ");
        //        }
        //        else if(i == status_idx) {
        //            printk("[status] ");
        //        }
        //        printk("index %d desc_addr 0x%x%x len 0x%x flags 0x%x next
        //        0x%x\n", i,
        //            u64_high_to_u32(desc_root[i].addr),
        //            u64_low_to_u32(desc_root[i].addr), desc_root[i].len,
        //            desc_root[i].flags,
        //            desc_root[i].next);
        //        if(i == header_idx) {
        //            printk("    req_type 0x%x req_sector 0x%x%x\n", req->type,
        //            u64_high_to_u32(req->sector),
        //            u64_low_to_u32(req->sector));
        //        }
        //        else if(i == data_idx) {
        //            printk("    req_write_addr 0x%x%x req_write_len 0x%x\n",
        //            u64_high_to_u32(req->data_bg),
        //            u64_low_to_u32(req->data_bg), req->len);
        //        }
        //        else if(i == status_idx) {
        //            printk("    req_status_addr 0x%x%x\n",
        //            u64_high_to_u32(req->status_addr),
        //            u64_low_to_u32(req->status_addr));
        //        }
    }

    // update used ring
    void *used = ipa2va(used_addr);
    vq->used = used;
    vq->used->flags = 0;

    // FIXME: real write len
    vq->used->ring[vq->used->idx % num].id = desc_chain_head_idx;
    vq->used->ring[vq->used->idx % num].len = req->len;

    //    printk("[*used_ring*]\n");

    //    for(int i = 0; i < num; i++) {
    //        if(i == vq->used->idx % num)
    //            printk("* ");
    //        printk("index %d id 0x%x len 0x%x\n", i, vq->used->ring[i].id,
    //        vq->used->ring[i].len);
    //    }

    // asm("dsb sy");
    vq->last_used_idx = vq->used->idx;
    vq->used->idx++;

    //    printf("cur_used_idx %d next_used_idx %d\n", vq->last_used_idx,
    //    vq->used->idx);

    // handle request

    void *buf = ipa2va(req->data_bg);

    blk_req_handler(req, buf);

    // v_m->dev->handler(req, buf);
}
