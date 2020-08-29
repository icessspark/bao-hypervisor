#include <at_utils.h>
#include <util.h>
#include <virt_dev.h>
#include <vm.h>

// FIXME: EXT4-fs (vda): initial error

extern spinlock_t req_handler_lock;

bool virt_dev_init(virtio_mmio_t* virtio_mmio)
{
    uint32_t type = virtio_mmio->type;
    // 分配virt_dev对象
    objcache_init(&virtio_mmio->dev_cache, sizeof(virt_dev_t), SEC_HYP_GLOBAL,
                  true);
    virt_dev_t* dev = objcache_alloc(&virtio_mmio->dev_cache);

    // 分配vq_cache对象并初始化
    objcache_init(&virtio_mmio->vq_cache, sizeof(virtq_t), SEC_HYP_GLOBAL,
                  true);
    virtq_t* vq = objcache_alloc(&virtio_mmio->vq_cache);
    virtq_init(vq);

    switch (type) {
        case VIRTIO_TYPE_BLOCK:

            // 分配blk_desc对象并初始化
            objcache_init(&dev->desc_cache, sizeof(blk_desc_t), SEC_HYP_GLOBAL,
                          true);
            blk_desc_t* blk = objcache_alloc(&dev->desc_cache);
            blk_cfg_init(blk);
            dev->desc = (void*)blk;

            // 初始化blk相关的features和req
            blk_features_init(&dev->features);
            objcache_init(&dev->req_cache, sizeof(struct virtio_blk_req),
                          SEC_HYP_GLOBAL, true);
            struct virtio_blk_req* req = objcache_alloc(&dev->req_cache);
            req->reserved = virtio_mmio->pa;
            dev->req = req;

            vq->notify_handler = process_guest_blk_notify;

            break;
        default:
            ERROR("Wrong virtio device type!\n");
    }

    // virtio_mmio和virt_dev、virtq相互关联
    virtio_mmio->dev = dev;
    virtio_mmio->vq = vq;

    // 设定virtio_mmio的handler类型
    virtio_mmio->handler = virtio_be_handler;

    return true;
}

void virt_dev_reset(virtio_mmio_t* v_m)
{
    v_m->regs.dev_stat = 0;
    v_m->regs.irt_stat = 0;
    v_m->regs.q_ready = 0;
    v_m->vq->ready = false;
}

void virtio_be_init_handler(virtio_mmio_t* virtio_mmio, emul_access_t* acc,
                            uint32_t offset, bool write)
{
    // printf("virtio_be_init_handler %s, address 0x%x \n",
    //        write == 1 ? "write" : "read", acc->addr);

    if (!write) {
        uint32_t value = 0;
        switch (offset) {
            case VIRTIO_MMIO_MAGIC_VALUE:
                value = virtio_mmio->regs.magic;
                break;
            case VIRTIO_MMIO_VERSION:
                value = virtio_mmio->regs.version;
                break;
            case VIRTIO_MMIO_DEVICE_ID:
                value = virtio_mmio->regs.device_id;
                break;
            case VIRTIO_MMIO_VENDOR_ID:
                value = virtio_mmio->regs.vendor_id;
                break;
            case VIRTIO_MMIO_HOST_FEATURES:
                if (virtio_mmio->regs.dev_feature_sel)
                    virtio_mmio->regs.dev_feature =
                        u64_high_to_u32(virtio_mmio->dev->features);
                else
                    virtio_mmio->regs.dev_feature =
                        u64_low_to_u32(virtio_mmio->dev->features);
                value = virtio_mmio->regs.dev_feature;
                break;
            case VIRTIO_MMIO_STATUS:
                value = virtio_mmio->regs.dev_stat;
                break;
            default:
                ERROR("virtio_be_init_handler wrong reg_read, address=0x%x",
                      acc->addr);
                return false;
        }
        vcpu_writereg(cpu.vcpu, acc->reg, value);
    } else {
        uint32_t value = vcpu_readreg(cpu.vcpu, acc->reg);
        switch (offset) {
            case VIRTIO_MMIO_HOST_FEATURES_SEL:
                virtio_mmio->regs.dev_feature_sel = value;
                break;
            case VIRTIO_MMIO_GUEST_FEATURES:
                virtio_mmio->regs.drv_feature = value;
                if (virtio_mmio->regs.drv_feature_sel)
                    virtio_mmio->driver_features |=
                        u32_to_u64_high(virtio_mmio->regs.drv_feature);
                else
                    virtio_mmio->driver_features |=
                        u32_to_u64_low(virtio_mmio->regs.drv_feature);
                break;
            case VIRTIO_MMIO_GUEST_FEATURES_SEL:
                virtio_mmio->regs.drv_feature_sel = value;
                break;
            case VIRTIO_MMIO_STATUS:
                virtio_mmio->regs.dev_stat = value;
                if (virtio_mmio->regs.dev_stat == 0) {
                    INFO("vm%d Virtio device %d reset", virtio_mmio->vm_id,
                         virtio_mmio->id);
                    virt_dev_reset(virtio_mmio);
                } else if (virtio_mmio->regs.dev_stat == 0xf) {
                    INFO("vm%d Virtio device %d init ok", virtio_mmio->vm_id,
                         virtio_mmio->id);
                }
                break;
            default:
                ERROR("virtio_be_init_handler wrong reg write 0x%x", acc->addr);
                return false;
        }
    }
    return true;
}

void virtio_be_queue_handler(virtio_mmio_t* virtio_mmio, emul_access_t* acc,
                             uint32_t offset, bool write)
{
    // printf("virtio_be_queue_handler %s, address 0x%x \n",
    //        write == 1 ? "write" : "read", acc->addr);

    if (!write) {
        uint32_t value = 0;
        switch (offset) {
            case VIRTIO_MMIO_QUEUE_NUM_MAX:
                value = virtio_mmio->regs.q_num_max;
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                value = virtio_mmio->regs.q_ready;
                break;
            default:
                ERROR("virtio_be_queue_handler wrong reg_read, address=0x%x",
                      acc->addr);
                return false;
        }
        vcpu_writereg(cpu.vcpu, acc->reg, value);
    } else {
        uint32_t value = vcpu_readreg(cpu.vcpu, acc->reg);
        switch (offset) {
            case VIRTIO_MMIO_QUEUE_SEL:
                virtio_mmio->regs.q_sel = value;
                break;
            case VIRTIO_MMIO_QUEUE_NUM:
                virtio_mmio->vq->num = value;
                break;
            case VIRTIO_MMIO_QUEUE_READY:
                virtio_mmio->vq->ready = value;
                virtio_mmio->regs.q_ready = value;
                INFO("vm%d virtio device %d Virtio queue %d init ok",
                     virtio_mmio->vm_id, virtio_mmio->id,
                     virtio_mmio->regs.q_sel);
                break;
            case VIRTIO_MMIO_QUEUE_DESC_LOW:
                virtio_mmio->regs.q_desc_l = value;
                break;
            case VIRTIO_MMIO_QUEUE_DESC_HIGH:
                virtio_mmio->regs.q_desc_h = value;
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_LOW:
                virtio_mmio->regs.q_drv_l = value;
                break;
            case VIRTIO_MMIO_QUEUE_AVAIL_HIGH:
                virtio_mmio->regs.q_drv_h = value;
                break;
            case VIRTIO_MMIO_QUEUE_USED_LOW:
                virtio_mmio->regs.q_dev_l = value;
                break;
            case VIRTIO_MMIO_QUEUE_USED_HIGH:
                virtio_mmio->regs.q_dev_h = value;
                break;
            default:
                ERROR("virtio_be_queue_handler wrong reg write 0x%x",
                      acc->addr);
                return false;
        }
    }
}

void virtio_be_cfg_handler(virtio_mmio_t* virtio_mmio, emul_access_t* acc,
                           uint32_t offset, bool write)
{
    // printf("virtio_be_cfg_handler %s, address 0x%x \n",
    //        write == 1 ? "write" : "read", acc->addr);

    if (!write) {
        uint32_t value = 0;
        switch (offset) {
            case VIRTIO_MMIO_CONFIG_GENERATION:
                value = virtio_mmio->dev->generation;
                break;
            case VIRTIO_MMIO_CONFIG ... VIRTIO_MMIO_REGS_END:
                value = *(uint64_t*)(virtio_mmio->dev->desc + offset -
                                     VIRTIO_MMIO_CONFIG);
                // printk("read VIRTIO_MMIO_CONFIG, offset 0x%x, value 0x%x\n",
                //        offset, value);
                break;
            default:
                ERROR("virtio_be_cfg_handler wrong reg_read, address=0x%x",
                      acc->addr);
                return false;
        }
        vcpu_writereg(cpu.vcpu, acc->reg, value);
    } else {
        uint32_t value = vcpu_readreg(cpu.vcpu, acc->reg);
        switch (offset) {
            default:
                ERROR("virtio_be_cfg_handler wrong reg write 0x%x", acc->addr);
                return false;
        }
    }
}

// TODO: reconsider the implement location
bool virtio_be_handler(emul_access_t* acc)
{
    spin_lock(&req_handler_lock);
    // printf("vm%d get lock\n", cpu.vcpu->vm->id);

    uint64_t addr = acc->addr;

    virtio_mmio_t* virtio_mmio = get_virt_mmio(addr);

    if (addr < virtio_mmio->va) {
        ERROR("virtio_emul_handler address error");
        return false;
    }

    uint32_t offset = addr - virtio_mmio->va;
    bool write = acc->write;

    if (offset == VIRTIO_MMIO_QUEUE_NOTIFY && write) {
        virtio_mmio->regs.irt_stat = 1;
        if (!virtio_mmio->vq->notify_handler(virtio_mmio->vq, virtio_mmio)) {
            ERROR("virtio_notify_handler error!");
        }
    } else if (offset == VIRTIO_MMIO_INTERRUPT_STATUS && !write) {
        vcpu_writereg(cpu.vcpu, acc->reg, virtio_mmio->regs.irt_stat);
    } else if (offset == VIRTIO_MMIO_INTERRUPT_ACK && write) {
        virtio_mmio->regs.irt_ack = vcpu_readreg(cpu.vcpu, acc->reg);
    } else if ((VIRTIO_MMIO_MAGIC_VALUE <= offset &&
                offset <= VIRTIO_MMIO_GUEST_FEATURES_SEL) ||
               offset == VIRTIO_MMIO_STATUS) {
        virtio_be_init_handler(virtio_mmio, acc, offset, write);
    } else if (VIRTIO_MMIO_QUEUE_SEL <= offset &&
               offset <= VIRTIO_MMIO_QUEUE_USED_HIGH) {
        virtio_be_queue_handler(virtio_mmio, acc, offset, write);
    } else if (VIRTIO_MMIO_CONFIG_GENERATION <= offset &&
               offset <= VIRTIO_MMIO_REGS_END) {
        virtio_be_cfg_handler(virtio_mmio, acc, offset, write);
    } else {
        ERROR("virtio_mmio regs wrong %s, address 0x%x",
              write == 1 ? "write" : "read", acc->addr);
    }

    spin_unlock(&req_handler_lock);
    // printf("vm%d release lock\n", cpu.vcpu->vm->id);
    return true;
}
