#include <virtq.h>
#include <at_utils.h>

static inline int next_desc(struct vring_desc *desc)
{
	return (!(desc->flags & VIRTQ_DESC_F_NEXT)) ? -1 : desc->next;
}

uint16_t get_avail_desc(virtq_t *vq, uint64_t avail_addr, uint32_t num) {

    void *avail = ipa2va(&cpu.as, avail_addr, 2, SEC_HYP_PRIVATE);
    vq->avail = avail;
    uint16_t avail_desc_idx = 0;

    printk("avail_ipa 0x%x%x\n", u64_high_to_u32(avail_addr), u64_low_to_u32(avail_addr));
    printk("avail_pa 0x%x%x\n", u64_high_to_u32(ipa2pa(avail_addr)), u64_low_to_u32(ipa2pa(avail_addr)));
    printk("avail_el2_va 0x%x%x\n", u64_high_to_u32((uint64_t)avail), u64_low_to_u32((uint64_t)avail));

    printk("avail_flags 0x%x idx 0x%x ring_addr 0x%x%x\n", vq->avail->flags, vq->avail->idx, 
                                                            u64_high_to_u32(vq->avail->ring), u64_low_to_u32(vq->avail->ring));

    if(vq->avail->idx == 0) {
        avail_desc_idx = vq->avail->ring[num-1];
        vq->last_avail_idx = avail_desc_idx;
        return avail_desc_idx;
    }

    for(int i=0; i<num; i++) {
        if(i == (vq->avail->idx - 1)/num) {
            printk("* ");
            avail_desc_idx = vq->avail->ring[i];
            vq->last_avail_idx = avail_desc_idx;
        }
        printk("index %d 0x%x\n", i, vq->avail->ring[i]);
    }

    return avail_desc_idx;
}

char buffer[10000];

void process_guest_blk_notify(virtq_t *vq, virtio_mmio_t *v_m) {
    uint64_t desc_root_addr = u32_to_u64_high(v_m->regs.q_desc_h) + u32_to_u64_low(v_m->regs.q_desc_l);
    uint64_t avail_addr = u32_to_u64_high(v_m->regs.q_drv_h) + u32_to_u64_low(v_m->regs.q_drv_l);
    uint64_t used_addr = u32_to_u64_high(v_m->regs.q_dev_h) + u32_to_u64_low(v_m->regs.q_dev_l);
    uint32_t num = vq->num;

    uint16_t avail_desc_idx = get_avail_desc(vq, avail_addr, num);

    printk("desc_root_ipa 0x%x%x\n", u64_high_to_u32(desc_root_addr), u64_low_to_u32(desc_root_addr));
    printk("desc_root_el2_pa 0x%x%x\n", u64_high_to_u32(ipa2pa(desc_root_addr)), u64_low_to_u32(ipa2pa(desc_root_addr)));

    struct vring_desc* desc_root = ipa2va(&cpu.as, desc_root_addr, 2, SEC_HYP_PRIVATE); 

    // header
    vq->desc = &desc_root[avail_desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;

    printk("[header] index %d desc_addr 0x%x%x len 0x%x flags 0x%x next 0x%x\n", avail_desc_idx, 
                                                                        u64_high_to_u32(desc_addr), u64_low_to_u32(desc_addr), 
                                                                        desc_len, 
                                                                        desc_flags, 
                                                                        desc_next);

    struct virtio_blk_req* req = v_m->dev->req;
    void* vreq = ipa2va(&cpu.as, desc_addr, 2, SEC_HYP_PRIVATE);

    req->type = *(uint32_t*)vreq;
    req->sector = *(uint64_t*)(vreq + 8);
    printk("    req_type 0x%x req_sector 0x%x%x\n", req->type, u64_high_to_u32(req->sector), u64_low_to_u32(req->sector));

    // DATA
    avail_desc_idx = desc_next;
    vq->desc = &desc_root[avail_desc_idx];
    desc_addr = vq->desc->addr;
    desc_len = vq->desc->len;
    desc_flags = vq->desc->flags;
    desc_next = vq->desc->next;
    
    printk("[data] index %d desc_addr 0x%x%x len 0x%x flags 0x%x next 0x%x\n", avail_desc_idx, 
                                                                        u64_high_to_u32(desc_addr), u64_low_to_u32(desc_addr), 
                                                                        desc_len, 
                                                                        desc_flags, 
                                                                        desc_next);
    req->data_bg = desc_addr;
    req->len = desc_len;
    printk("    req_write_addr 0x%x%x req_write_len 0x%x\n", u64_high_to_u32(req->data_bg), u64_low_to_u32(req->data_bg), req->len);

    //status
    avail_desc_idx = desc_next;
    vq->desc = &desc_root[avail_desc_idx];
    desc_addr = vq->desc->addr;
    desc_len = vq->desc->len;
    desc_flags = vq->desc->flags;
    desc_next = vq->desc->next;

    printk("[status] index %d desc_addr 0x%x%x len 0x%x flags 0x%x next 0x%x\n", avail_desc_idx, 
                                                                    u64_high_to_u32(desc_addr), u64_low_to_u32(desc_addr), 
                                                                    desc_len, 
                                                                    desc_flags, 
                                                                    desc_next);

    req->status_addr = desc_addr;
    printk("    req_status_addr 0x%x%x\n", u64_high_to_u32(req->status_addr), u64_low_to_u32(req->status_addr));


    printf("fun_pointer 0x%lx\n", v_m->dev->handler);

    void *buf = ipa2va(&cpu.as, req->data_bg, 2, SEC_HYP_PRIVATE);
    
    blk_req_handler(req, buf);

    
    // v_m->dev->handler(req, buf);

}

