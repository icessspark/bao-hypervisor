#include <at_utils.h>
#include <drivers/virtio_prelude.h>
#include <string.h>
#include <virt_blk.h>

void blk_features_init(uint64_t *features)
{
    *features |=
        VIRTIO_F_VERSION_1; /*a simple way to detect legacy devices or drivers*/
    *features |= VIRTIO_BLK_F_SIZE_MAX;
    // *features |= VIRTIO_BLK_F_SIZE_MAX;    /* Indicates maximum segment size
    // */
    *features |= VIRTIO_BLK_F_SEG_MAX; /* Indicates maximum # of segments */
    // *features |= VIRTIO_BLK_F_GEOMETR;        /* Legacy geometry available */
    // *features |= VIRTIO_BLK_F_RO;             /* Disk is read-only */
    // *features |= VIRTIO_BLK_F_BLK_SIZE;       /* Block size of disk is
    // available*/
    // TODO: add flush support
    // *features |= VIRTIO_BLK_F_FLUSH;          /* Flush command supported */
    // *features |= VIRTIO_BLK_F_TOPOLOGY;       /* Topology information is
    // available */ *features |= VIRTIO_BLK_F_CONFIG_WCE;     /* Writeback mode
    // available in config */ *features |= VIRTIO_BLK_F_DISCARD;        /* Trim
    // blocks */ *features |= VIRTIO_BLK_F_WRITE_ZEROES;   /* Write zeros */
}
// TODO: complete blk cfg
void blk_cfg_init(blk_desc_t *blk_cfg)
{
    blk_cfg->capacity = 2000 * 1024 * 1024 / SECTOR_BSIZE;
    blk_cfg->size_max = 0x1000; /* not negotiated */
    blk_cfg->seg_max = BLOCKIF_IOV_MAX;
    // blk_cfg->geometry.cylinders = 0; /* no geometry */
    // blk_cfg->geometry.heads = 0;
    // blk_cfg->geometry.sectors = 0;
    // blk_cfg->blk_size = sectsz;
    // blk_cfg->topology.physical_block_exp =
    //     (sts > sectsz) ? (ffs(sts / sectsz) - 1) : 0;
    // blk_cfg->topology.alignment_offset =
    //     (sto != 0) ? ((sts - sto) / sectsz) : 0;
    // blk_cfg->topology.min_io_size = 0;
    // blk_cfg->topology.opt_io_size = 0;
    // blk_cfg->writeback = blockif_get_wce(blk->bc);
    /* ... */
}

void blk_req_handler(void *req, void *buffer)
{
    struct virtio_blk_req *blk_req = req;

    uint64_t sector = blk_req->sector;
    uint32_t len = blk_req->len / SECTOR_BSIZE;
    uint32_t offset = blk_req->reserved / SECTOR_BSIZE;

    if (blk_req->type == VIRTIO_BLK_T_IN) {
        // printf("[C%d][vm%d][R] sector %08lx, offset 0x%x, len %04x\n",
        // cpu.id,
        //        cpu.vcpu->vm->id, sector, offset, len);
        virtio_blk_read(sector + offset, len, buffer);
    } else if (blk_req->type == VIRTIO_BLK_T_OUT) {
        // printf("[C%d][vm%d][W] sector %08lx, offset 0x%x, len %04x\n",
        // cpu.id,
        //        cpu.id, cpu.vcpu->vm->id, sector, offset, len);
        virtio_blk_write(sector + offset, len, buffer);
    } else if (blk_req->type != 8) {
        ERROR("Wrong block request type %d ", blk_req->type);
    }
}

bool process_guest_blk_notify(virtq_t *vq, virtio_mmio_t *v_m)
{
    if (!vq->ready) {
        WARNING("Virt_queue is not ready!");
        return false;
    }
    uint64_t desc_table_addr = u32_to_u64_high(v_m->regs.q_desc_h) |
                               u32_to_u64_low(v_m->regs.q_desc_l);

    // printf("[DEBUG][vm%d] desc_table_addr 0x%lx\n", v_m->vm_id,
    // desc_table_addr);
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

        // printf("[DEBUG][vm%d] req->data_bg 0x%lx\n", v_m->vm_id,
        // req->data_bg);

        // handle request
        void *buf = ipa2va(req->data_bg);
        if (req->type == VIRTIO_BLK_T_GET_ID) {
            char *dev_id = buf;
            char *v_m_id = (char *)mem_alloc_page(1, SEC_HYP_VM, true);
            strcpy(dev_id, "virtio_blk");
            strcat(dev_id, itostr(v_m_id, v_m->id) + '\0');
            INFO("Block request type VIRTIO_BLK_T_GET_ID, ID %s", dev_id);
        } else if (req->type > 1) {
            update_virt_desc_status(vq, req, desc_next, desc_table,
                                    VIRTIO_BLK_S_UNSUPP);
        }

        // FIXME: decide on how to realize handler
        blk_req_handler(req, buf);

        // FIXME: should free ppages?
        mem_free_vpage(&cpu.as, desc_table, 1, false);
        mem_free_vpage(&cpu.as, vq->desc, 1, false);
        mem_free_vpage(&cpu.as, vq->avail, 1, false);
        mem_free_vpage(&cpu.as, vq->used, 1, false);
        mem_free_vpage(&cpu.as, buf, 1, false);

        process_count++;
    }

    virtq_notify(vq, process_count, v_m->int_id);

    return true;
}
