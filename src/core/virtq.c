#include <virtq.h>
#include <at_utils.h>

#define DEBUG_ON 1

static inline int next_desc(struct vring_desc *desc)
{
	return (!(desc->flags & VIRTQ_DESC_F_NEXT)) ? -1 : desc->next;
}

uint16_t get_avail_desc(virtq_t *vq, uint64_t avail_addr, uint32_t num) {

    void *avail = ipa2va(&cpu.as, avail_addr, 2, SEC_HYP_PRIVATE);
    vq->avail = avail;
    uint16_t avail_desc_idx = 0;

    DEBUG("[*avail_ring*]\n");

    DEBUG("avail_flags 0x%x idx 0x%x ring_addr 0x%lx\n", vq->avail->flags, vq->avail->idx, 
        vq->avail->ring);
    
    DEBUG("last_avail_idx %d cur_avail_idx %d\n", vq->last_avail_idx, vq->avail->idx);
    
    if(vq->avail->idx <= vq->last_avail_idx) {
        WARNING("available idx is not changed\n");
        return num+1;
    }

    vq->last_avail_idx = vq->avail->idx;

    if(vq->avail->idx == 0) {
        avail_desc_idx = vq->avail->ring[num-1];
        vq->last_avail_idx = vq->avail->idx;
        return avail_desc_idx;
    }

    for(int i=0; i<num; i++) {
        if(i == (vq->avail->idx-1) % num) {
            DEBUG("* ");
            avail_desc_idx = vq->avail->ring[i];
        }
        DEBUG("index %d 0x%x\n", i, vq->avail->ring[i]);
    }

    return avail_desc_idx;
}


void process_guest_blk_notify(virtq_t *vq, virtio_mmio_t *v_m) {
    uint64_t desc_root_addr = u32_to_u64_high(v_m->regs.q_desc_h) + u32_to_u64_low(v_m->regs.q_desc_l);
    uint64_t avail_addr = u32_to_u64_high(v_m->regs.q_drv_h) + u32_to_u64_low(v_m->regs.q_drv_l);
    uint64_t used_addr = u32_to_u64_high(v_m->regs.q_dev_h) + u32_to_u64_low(v_m->regs.q_dev_l);
    uint32_t num = vq->num;

    uint16_t avail_desc_idx = get_avail_desc(vq, avail_addr, num);
    if(avail_desc_idx >=num) {
        WARNING("unable to get desc_chain\n");
        return;
    }
    uint16_t desc_chain_head_idx = avail_desc_idx;

    struct vring_desc* desc_root = ipa2va(&cpu.as, desc_root_addr, 2, SEC_HYP_PRIVATE); 

    DEBUG("[*desc_ring*]\n");

    // header
    int header_idx = avail_desc_idx; // for print
    vq->desc = &desc_root[avail_desc_idx];
    uint64_t desc_addr = vq->desc->addr;
    uint32_t desc_len = vq->desc->len;
    uint16_t desc_flags = vq->desc->flags;
    uint16_t desc_next = vq->desc->next;

    struct virtio_blk_req* req = v_m->dev->req;
    void* vreq = ipa2va(&cpu.as, desc_addr, 2, SEC_HYP_PRIVATE);

    req->type = *(uint32_t*)vreq;
    req->sector = *(uint64_t*)(vreq + 8);

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

    //status
    avail_desc_idx = desc_next;
    int status_idx = avail_desc_idx;  // for print
    vq->desc = &desc_root[avail_desc_idx];
    desc_addr = vq->desc->addr;
    desc_len = vq->desc->len;
    desc_flags = vq->desc->flags;
    desc_next = vq->desc->next;

    req->status_addr = desc_addr;

    void *state = ipa2va(&cpu.as, req->status_addr, 2, SEC_HYP_PRIVATE);
    *(uint32_t*)state = 0;

    for(int i = 0; i < num; i++) {
        if(i == header_idx) {
            DEBUG("[header] ");
        } 
        else if(i == data_idx) {
            DEBUG("[data] ");
        }
        else if(i == status_idx) {
            DEBUG("[status] ");
        }
        DEBUG("index %d desc_addr 0x%lx len 0x%x flags 0x%x next 0x%x\n", i, 
            desc_root[i].addr, 
            desc_root[i].len, 
            desc_root[i].flags, 
            desc_root[i].next);
        if(i == header_idx) {
            DEBUG("    req_type 0x%x req_sector 0x%lx\n", req->type, req->sector);
        }
        else if(i == data_idx) {
            DEBUG("    req_write_addr 0x%lx req_write_len 0x%x\n", req->data_bg, req->len);
        }
        else if(i == status_idx) {
            DEBUG("    req_status_addr 0x%lx\n", req->status_addr);
        }
    }


    // update used ring
    void *used = ipa2va(&cpu.as, used_addr, 2, SEC_HYP_PRIVATE);
    vq->used = used;
    vq->used->flags = 0;

    // FIXME: real write len
    vq->used->ring[vq->used->idx % num].id = desc_chain_head_idx;
    vq->used->ring[vq->used->idx % num].len = req->len;
    
    DEBUG("[*used_ring*]\n");

    for(int i = 0; i < num; i++) {
        if(i == vq->used->idx % num)
            DEBUG("* ");
        DEBUG("index %d id 0x%x len 0x%x\n", i, vq->used->ring[i].id, vq->used->ring[i].len);
    }

    // asm("dsb sy");
    vq->last_used_idx = vq->used->idx;
    vq->used->idx ++;
    
    DEBUG("cur_used_idx %d next_used_idx %d\n", vq->last_used_idx, vq->used->idx);

    // handle request

    void *buf = ipa2va(&cpu.as, req->data_bg, 2, SEC_HYP_PRIVATE);
    
    blk_req_handler(req, buf);

    // v_m->dev->handler(req, buf);

}

