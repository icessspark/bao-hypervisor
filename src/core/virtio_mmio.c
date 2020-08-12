#include <bao.h>
#include <virtio_mmio.h>

virtio_mmio_manager_t virtio_mmio_manager = { .num = 0 };
spinlock_t req_handler_lock = SPINLOCK_INITVAL;

void virtio_init(vm_t* vm) {

    INFO("virtio_init");

    add_virtio_mmio();

    for(int i=0;i<virtio_mmio_manager.num;i++) {
        virtio_mmio_t* virtio_mmio = &virtio_mmio_manager.virt_mmio_devs[i];
        virtio_mmio_init(virtio_mmio);
        emul_region_t emu = {
            .va_base = virtio_mmio->va,
            .pa_base = virtio_mmio->pa,
            .size = virtio_mmio->size,
            .handler = virtio_mmio->handler
        };
        vm_add_emul(vm, &emu);
    }
}

// TODO: enable virtio_mmio config
void add_virtio_mmio() {
    virtio_mmio_t virtio_blk = {
        .id = 0,
        .va = VIRTIO_MMIO_ADDRESS,
        .pa = 0,
        .size = 0x400,
        .type = VIRTIO_TYPE_BLOCK
    };
    virtio_mmio_manager.virt_mmio_devs[0] = virtio_blk;
    virtio_mmio_manager.num++;
}

bool virtio_mmio_init(virtio_mmio_t* virtio_mmio) {
    virtio_mmio->regs.magic = VIRTIO_MMIO_MAGIG;
    virtio_mmio->regs.version = VIRTIO_VERSION;
    virtio_mmio->regs.vendor_id = VIRTIO_VENDER_ID;
    virtio_mmio->regs.device_id = virtio_mmio->type;
    virtio_mmio->regs.dev_feature = 0;
    virtio_mmio->regs.drv_feature = 0;
    virtio_mmio->regs.q_num_max = VIRTQUEUE_MAX_SIZE;

    if(!virt_dev_init(virtio_mmio)) {
        ERROR("virt_device init error!");
        return false;
    }

}