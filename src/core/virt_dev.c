#include <at_utils.h>
#include <drivers/virtio_prelude.h>
#include <printf.h>
#include <util.h>
#include <virt_dev.h>

// FIXME: EXT4-fs (vda): initial error

extern spinlock_t req_handler_lock;

void blk_features_init(uint64_t* features)
{
    *features |= VIRTIO_F_VERSION_1;          /*a simple way to detect legacy devices or drivers*/
    *features |= VIRTIO_BLK_F_SIZE_MAX;
    // *features |= VIRTIO_BLK_F_SIZE_MAX;    /* Indicates maximum segment size */
    *features |= VIRTIO_BLK_F_SEG_MAX;        /* Indicates maximum # of segments */
    // *features |= VIRTIO_BLK_F_GEOMETR;        /* Legacy geometry available */
    // *features |= VIRTIO_BLK_F_RO;             /* Disk is read-only */
    // *features |= VIRTIO_BLK_F_BLK_SIZE;       /* Block size of disk is available*/
    // TODO: add flush support
    // *features |= VIRTIO_BLK_F_FLUSH;          /* Flush command supported */
    // *features |= VIRTIO_BLK_F_TOPOLOGY;       /* Topology information is available */ 
    // *features |= VIRTIO_BLK_F_CONFIG_WCE;     /* Writeback mode available in config */ 
    // *features |= VIRTIO_BLK_F_DISCARD;        /* Trim blocks */ 
    // *features |= VIRTIO_BLK_F_WRITE_ZEROES;   /* Write zeros */
}

bool virt_dev_init(virtio_mmio_t* virtio_mmio)
{
    uint32_t type = virtio_mmio->type;

    switch (type) {
        case VIRTIO_TYPE_BLOCK:
            // 分配virt_dev对象给virtio_mmio
            objcache_init(&virtio_mmio->dev_cache, sizeof(virt_dev_t),
                          SEC_HYP_GLOBAL, true);
            virt_dev_t* dev = objcache_alloc(&virtio_mmio->dev_cache);

            // 分配blk_desc对象并初始化
            objcache_init(&dev->desc_cache, sizeof(blk_desc_t), SEC_HYP_GLOBAL,
                          true);
            blk_desc_t* blk = objcache_alloc(&dev->desc_cache);
            blk_cfg_init(blk);
            dev->desc = (void*)blk;
            dev->generation = 0;
            dev->type = type;

            // 初始化blk相关的features和req
            blk_features_init(&dev->features);
            objcache_init(&dev->req_cache, sizeof(struct virtio_blk_req),
                          SEC_HYP_GLOBAL, true);
            struct virtio_blk_req* req = objcache_alloc(&dev->req_cache);
            req->reserved = 0;

            // 分配vq_cache对象并初始化
            objcache_init(&virtio_mmio->vq_cache, sizeof(virtq_t),
                          SEC_HYP_GLOBAL, true);
            virtq_t* vq = objcache_alloc(&virtio_mmio->vq_cache);
            virtq_init(vq) ;
            vq->notify_handler = process_guest_blk_notify;

            // virtio_mmio和virt_dev、virtq相互关联
            virtio_mmio->dev = dev;
            virtio_mmio->vq = vq;

            // 设定virtio_mmio的handler类型
            virtio_mmio->handler = virtio_be_blk_handler;

            break;
        default:
            ERROR("Wrong virtio device type!\n");
            break;
    }

    return true;
}

// TODO: complete blk cfg
void blk_cfg_init(blk_desc_t* blk_cfg)
{
    blk_cfg->capacity = 2000 * 1024 * 1024 / 512;
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

void virt_dev_reset(virtio_mmio_t* v_m)
{
    v_m->regs.dev_stat = 0;
    v_m->regs.irt_stat = 0;
    v_m->regs.q_ready = 0;
}

// TODO: reconsider the implement location
bool virtio_be_blk_handler(emul_access_t* acc)
{
    uint64_t addr = acc->addr;

    virtio_mmio_t* virtio_mmio = get_virt_mmio(addr);

    spin_lock(&req_handler_lock);

    //    INFO("virtio_emul_handler addr 0x%x %s ", addr, acc->write ? "write to
    //    host" : "read from host");

    if (addr < VIRTIO_MMIO_ADDRESS) {
        ERROR("virtio_emul_handler address error");
        return false;
    }

    uint32_t offset = addr - VIRTIO_MMIO_ADDRESS;

    if (!acc->write) {
        uint32_t value = 0;
        switch (offset) {
            case VIRTIO_MMIO_MAGIC_VALUE:
                value = virtio_mmio->regs.magic;
                break;
            case VIRTIO_MMIO_VERSION:
                value = virtio_mmio->regs.version;
                // printk("read VIRTIO_MMIO_VERSION 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_DEVICE_ID:
                value = virtio_mmio->regs.device_id;
                // printk("read VIRTIO_MMIO_DEVICE_ID 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_VENDOR_ID:
                value = virtio_mmio->regs.vendor_id;
                break;
            case VIRTIO_MMIO_STATUS:
                value = virtio_mmio->regs.dev_stat;
                // printk("read VIRTIO_MMIO_STATUS 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_HOST_FEATURES:
                if (virtio_mmio->regs.dev_feature_sel)
                    virtio_mmio->regs.dev_feature =
                        u64_high_to_u32(virtio_mmio->dev->features);
                else
                    virtio_mmio->regs.dev_feature =
                        u64_low_to_u32(virtio_mmio->dev->features);
                value = virtio_mmio->regs.dev_feature;
                // printk("read VIRTIO_MMIO_HOST_FEATURES 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NUM_MAX:
                value = virtio_mmio->regs.q_num_max;
                // printk("read VIRTIO_MMIO_QUEUE_NUM_MAX 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                value = virtio_mmio->regs.q_ready;
                // printk("read VIRTIO_MMIO_QUEUE_READY 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_INTERRUPT_STATUS:
                // FIXME: VIRTIO_MMIO_INTERRUPT_STATUS
                value = virtio_mmio->regs.irt_stat;
                // printk("read VIRTIO_MMIO_INTERRUPT_STATUS 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_CONFIG_GENERATION:
                value = virtio_mmio->dev->generation;
                // printk("read VIRTIO_MMIO_CONFIG_GENERATION 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_CONFIG ... VIRTIO_MMIO_REGS_END:
                value = *(uint64_t*)(virtio_mmio->dev->desc + offset -
                                     VIRTIO_MMIO_CONFIG);
                // printk("read VIRTIO_MMIO_CONFIG, offset 0x%x, value
                // 0x%x\n\r",
                //        offset, value);
                break;
            default:
                ERROR("virtio_emul_handler wrong reg_read, address=0x%x", addr);
                return false;
        }

        vcpu_writereg(cpu.vcpu, acc->reg, value);
    } else {
        uint32_t value = vcpu_readreg(cpu.vcpu, acc->reg);
        switch (offset) {
            case VIRTIO_MMIO_STATUS:
                virtio_mmio->regs.dev_stat = value;
                // printk("write device_state 0x%x\n\r", value);
                if (virtio_mmio->regs.dev_stat == 0) {
                    virt_dev_reset(virtio_mmio);
                }
                break;
            case VIRTIO_MMIO_HOST_FEATURES_SEL:
                virtio_mmio->regs.dev_feature_sel = value;
                // printk("write dev_feature_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_GUEST_FEATURES_SEL:
                virtio_mmio->regs.drv_feature_sel = value;
                // printk("write drv_feature_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_GUEST_FEATURES:
                virtio_mmio->regs.drv_feature = value;
                // printk("write VIRTIO_MMIO_GUEST_FEATURES 0x%x!\n\r", value);
                if (virtio_mmio->regs.drv_feature_sel)
                    virtio_mmio->driver_features |=
                        u32_to_u64_high(virtio_mmio->regs.drv_feature);
                else
                    virtio_mmio->driver_features |=
                        u32_to_u64_low(virtio_mmio->regs.drv_feature);
                break;
            case VIRTIO_MMIO_QUEUE_SEL:
                virtio_mmio->regs.q_sel = value;
                // printk("write q_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NUM:
                virtio_mmio->vq->num = value;
                // printk("write q_num 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NOTIFY:
                // printk("write q_notify 0x%x\n\r", value);
                virtio_mmio->regs.q_notify = value;
                virtio_mmio->regs.irt_stat = 1;
                if (!virtio_mmio->vq->notify_handler(virtio_mmio->vq,
                                                     virtio_mmio)) {
                    ERROR("[Virtio] Notify_handler error!");
                }
                break;
            case VIRTIO_MMIO_INTERRUPT_ACK:
                virtio_mmio->regs.irt_ack = value;
                // printk("write irt_ack 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_DESC_LOW:
                virtio_mmio->regs.q_desc_l = value;
                // printk("write q_desc_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_DESC_HIGH:
                virtio_mmio->regs.q_desc_h = value;
                // printk("write q_desc_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
                virtio_mmio->regs.q_drv_l = value;
                // printk("write q_drv_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
                virtio_mmio->regs.q_drv_h = value;
                // printk("write q_drv_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_USED_LOW:
                virtio_mmio->regs.q_dev_l = value;
                // printk("write q_dev_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_USED_HIGH:
                virtio_mmio->regs.q_dev_h = value;
                // printk("write q_dev_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                virtio_mmio->regs.q_ready = value;
                // printk("write q_ready 0x%x\n\r", value);
                break;
            default:
                ERROR("[Virtio] emul_handler wrong reg write 0x%x", addr);
                return false;
        }
    }

    spin_unlock(&req_handler_lock);

    return true;
}

void blk_req_handler(void* req, void* buffer)
{
    struct virtio_blk_req* blk_req = req;

    uint64_t sector = blk_req->sector;
    uint32_t len = blk_req->len;

    if (blk_req->type == VIRTIO_BLK_T_IN) {
        // printf("[C%d][BLK][R] sector %08lx, len %04x\n", cpu.id, sector, len);
        virtio_blk_read(sector, len / SECTOR_BSIZE, buffer);
    } else if (blk_req->type == VIRTIO_BLK_T_OUT) {
        // printf("[C%d][BLK][W] sector %08lx, len %04x\n", cpu.id, sector, len);
        virtio_blk_write(sector, len / SECTOR_BSIZE, buffer);
    } else if (blk_req->type != 8) {
        ERROR("Wrong block request type %d ", blk_req->type);
    }
}
