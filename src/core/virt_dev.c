#include <virt_dev.h>
#include <util.h>
#include <at_utils.h>

void do_notify(uint64_t addr, uint32_t num) {
    
    printk("ipa 0x%x%x\n", u64_high_to_u32(addr), u64_low_to_u32(addr));
    printk("el2_pa 0x%x%x\n", u64_high_to_u32(ipa2pa(addr)), u64_low_to_u32(ipa2pa(addr)));

    ppages_t p = mem_ppages_get(ipa2pa(addr), 2);
    void *desc = mem_alloc_vpage(&cpu.as, SEC_HYP_PRIVATE, NULL, 2);
    mem_map(&cpu.as, desc, &p, 2, PTE_HYP_FLAGS);

    struct vring_desc *vdesc = desc; 
    struct vring_avail *vavil = desc + 16 * num; 
    struct vring_used *vused = desc + PAGE_SIZE;
    
    for(int i=0;i<num;i++) {
        printk("index %d addr 0x%x%x len 0x%x flags 0x%x next 0x%x\n", 
            i, u64_high_to_u32(vdesc[i].addr), u64_low_to_u32(vdesc[i].addr), 
            vdesc[i].len, vdesc[i].flags, vdesc[i].next);
    }
}


bool virt_dev_init(virtio_mmio_t* virtio_mmio) {
    uint32_t type = virtio_mmio->type;

    switch (type) {
        case VIRTIO_TYPE_BLOCK:
            // 分配virt_dev对象给virtio_mmio
            objcache_init(&virtio_mmio->dev_cache, sizeof(virt_dev_t), SEC_HYP_GLOBAL,true);
            virt_dev_t *dev = objcache_alloc(&virtio_mmio->dev_cache);
            
            // 分配blk_desc对象并初始化
            objcache_init(&dev->desc_cache, sizeof(blk_desc_t), SEC_HYP_GLOBAL, true);
            blk_desc_t *blk = objcache_alloc(&dev->desc_cache);
            blk_cfg_init(blk);
            dev->desc = (void *) blk;
            dev->generation = 0;

            // 分配vq_desc_cache对象并初始化
            objcache_init(&dev->vq_cache, sizeof(virq_t), SEC_HYP_GLOBAL, true);
            virq_t *vq = objcache_alloc(&dev->vq_cache);
            dev->vq = vq;
            virtio_mmio->regs.q_num_max = VIRTQUEUE_MAX_SIZE;

            // 初始化blk相关的features
            blk_features_init(&dev->features);

            // 初始化req
            objcache_init(&virtio_mmio->req_cache, sizeof(struct virtio_blk_req), SEC_HYP_GLOBAL, true);
            struct virtio_blk_req *req = objcache_alloc(&virtio_mmio->req_cache);
            virtio_mmio->req = req;

            // virt_dev和virtio_mmio相互关联
            virtio_mmio->dev = dev;
            dev->master = virtio_mmio;
            dev->type = type;

            // 设定virtio_mmio的handler类型
            dev->master->handler = virtio_be_blk_handler;

            break;
        default:
            ERROR("Wrong virtio device type!\n");
            break;
    }

    return true;
}

void blk_features_init(uint64_t* features) {
    *features |= VIRTIO_F_VERSION_1;          // This indicates compliance with this specification, 
                                              // giving a simple way to detect legacy devices or drivers.
    // *features |= VIRTIO_BLK_F_SIZE_MAX;       /* Indicates maximum segment size */
    *features |= VIRTIO_BLK_F_SEG_MAX;        /* Indicates maximum # of segments */
    // *features |= VIRTIO_BLK_F_GEOMETR;        /* Legacy geometry available */
    *features |= VIRTIO_BLK_F_RO;             /* Disk is read-only */
    // *features |= VIRTIO_BLK_F_BLK_SIZE;       /* Block size of disk is available*/
    // *features |= VIRTIO_BLK_F_FLUSH;          /* Flush command supported */
    // *features |= VIRTIO_BLK_F_TOPOLOGY;       /* Topology information is available */
    // *features |= VIRTIO_BLK_F_CONFIG_WCE;     /* Writeback mode available in config */
    // *features |= VIRTIO_BLK_F_DISCARD;        /* Trim blocks */
    // *features |= VIRTIO_BLK_F_WRITE_ZEROES;   /* Write zeros */
}

// TODO: complete blk cfg
void blk_cfg_init(blk_desc_t *blk_cfg) {
    uint32_t size = 100*1024*1024;
    blk_cfg->capacity = size / SECTOR_BSIZE;
	blk_cfg->size_max = 0;	/* not negotiated */
	blk_cfg->seg_max = BLOCKIF_IOV_MAX;
	blk_cfg->geometry.cylinders = 0;	/* no geometry */
	blk_cfg->geometry.heads = 0;
	blk_cfg->geometry.sectors = 0;
	// blk_cfg->blk_size = sectsz;
    // blk_cfg->topology.physical_block_exp =
	//     (sts > sectsz) ? (ffs(sts / sectsz) - 1) : 0;
	// blk_cfg->topology.alignment_offset =
	//     (sto != 0) ? ((sts - sto) / sectsz) : 0;
	blk_cfg->topology.min_io_size = 0;
	blk_cfg->topology.opt_io_size = 0;
	// blk_cfg->writeback = blockif_get_wce(blk->bc);

    /* ... */
}

void virt_dev_reset(virt_dev_t* dev){
    dev->master->regs.dev_stat = 0;
    dev->master->regs.irt_stat = 0;
    dev->master->regs.q_ready = 0;
}

// TODO: reconsider the implement location
bool virtio_be_blk_handler(emul_access_t *acc) {
    uint64_t addr = acc->addr;
    INFO("virtio_emul_handler addr 0x%x %s ", addr, acc->write ? "write to host" : "read from host");

    virtio_mmio_t* virtio_mmio = get_virt_mmio(addr);

    if(addr < VIRTIO_MMIO_ADDRESS) {
        ERROR("virtio_emul_handler address error");
        return false;
    }

    uint32_t offset = addr - VIRTIO_MMIO_ADDRESS;
    
    if(!acc->write) {
        uint32_t value = 0;
        switch (offset)
        {
            case VIRTIO_MMIO_MAGIC_VALUE:
                value = virtio_mmio->regs.magic;
                break;
            case VIRTIO_MMIO_VERSION:
                value = virtio_mmio->regs.version;
                printk("read VIRTIO_MMIO_VERSION 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_DEVICE_ID:
                value = virtio_mmio->regs.device_id;
                printk("read VIRTIO_MMIO_DEVICE_ID 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_VENDOR_ID:
                value = virtio_mmio->regs.vendor_id;
                break;
            case VIRTIO_MMIO_STATUS:
                value = virtio_mmio->regs.dev_stat;
                printk("read VIRTIO_MMIO_STATUS 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_HOST_FEATURES:
                if(virtio_mmio->regs.dev_feature_sel)
                    virtio_mmio->regs.dev_feature = u64_high_to_u32(virtio_mmio->dev->features);
                else
                    virtio_mmio->regs.dev_feature = u64_low_to_u32(virtio_mmio->dev->features);
                value = virtio_mmio->regs.dev_feature;
                printk("read VIRTIO_MMIO_HOST_FEATURES 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NUM_MAX:
                value = virtio_mmio->regs.q_num_max;
                printk("read VIRTIO_MMIO_QUEUE_NUM_MAX 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                value = virtio_mmio->regs.q_ready;
                printk("read VIRTIO_MMIO_QUEUE_READY 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_CONFIG_GENERATION:
                value = virtio_mmio->dev->generation;
                printk("read VIRTIO_MMIO_CONFIG_GENERATION 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_CONFIG ... VIRTIO_MMIO_REGS_END:
                value = *(uint64_t*)(virtio_mmio->dev->desc + offset - VIRTIO_MMIO_CONFIG);
                printk("read VIRTIO_MMIO_CONFIG, offset 0x%x, value 0x%x\n\r", offset, value);
                break;
            default:
                ERROR("virtio_emul_handler wrong reg_read, address=0x%x", addr);
                return false;
        }
        
        vcpu_writereg(cpu.vcpu, acc->reg, value);
    } else {
        uint32_t value = vcpu_readreg(cpu.vcpu, acc->reg);
        switch (offset)
        {
            case VIRTIO_MMIO_STATUS:
                virtio_mmio->regs.dev_stat = value;
                printk("write device_state 0x%x\n\r", value);
                if(virtio_mmio->regs.dev_stat == 0) {
                    virt_dev_reset(virtio_mmio->dev);
                }
                break;
            case VIRTIO_MMIO_HOST_FEATURES_SEL:
                virtio_mmio->regs.dev_feature_sel = value;
                printk("write dev_feature_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_GUEST_FEATURES_SEL:
                virtio_mmio->regs.drv_feature_sel = value;
                printk("write drv_feature_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_GUEST_FEATURES:
                virtio_mmio->regs.drv_feature = value;
                printk("write VIRTIO_MMIO_GUEST_FEATURES 0x%x!\n\r", value);
                if(virtio_mmio->regs.drv_feature_sel)
                    virtio_mmio->driver_features |= u32_to_u64_high(virtio_mmio->regs.drv_feature);
                else
                    virtio_mmio->driver_features |= u32_to_u64_low(virtio_mmio->regs.drv_feature);
                break;
            case VIRTIO_MMIO_QUEUE_SEL:
                virtio_mmio->regs.q_sel = value;
                printk("write q_sel 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NUM:
                virtio_mmio->regs.q_num = value;
                printk("write q_num 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_NOTIFY:
                virtio_mmio->regs.q_notify = value;
                printk("write q_notify 0x%x\n\r", value);             
                do_notify(u32_to_u64_high(virtio_mmio->regs.q_desc_h) | u32_to_u64_low(virtio_mmio->regs.q_desc_l), virtio_mmio->regs.q_num);
                break;
            case VIRTIO_MMIO_QUEUE_DESC_LOW:
                virtio_mmio->regs.q_desc_l = value;
                printk("write q_desc_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_DESC_HIGH:
                virtio_mmio->regs.q_desc_h = value;
                printk("write q_desc_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
                virtio_mmio->regs.q_drv_l = value;
                printk("write q_drv_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
                virtio_mmio->regs.q_drv_h = value;
                printk("write q_drv_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_USED_LOW:
                virtio_mmio->regs.q_dev_l = value;
                printk("write q_dev_l 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_USED_HIGH:
                virtio_mmio->regs.q_dev_h = value;
                printk("write q_dev_h 0x%x\n\r", value);
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                virtio_mmio->regs.q_ready = value;
                printk("write q_ready 0x%x\n\r", value);
                break;
            default:
                ERROR("virtio_emul_handler wrong reg write 0x%x", addr);
                return false;
        }
    }

    return true;
}