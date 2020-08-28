#include <bao.h>
#include <util.h>
#include <virtio_mmio.h>

// TODO: share blk between VMs
// TODO remove size_max and seg_max restriction

virtio_mmio_manager_t virtio_mmio_manager = {
    .num = 0, .vm_has_dev = 0, .devs_probed = false};
spinlock_t req_handler_lock = SPINLOCK_INITVAL;

// TODO: need more flexible way to config virtio_mmio
static void manager_add_virtio_mmio()
{
    if (virtio_mmio_manager.devs_probed) return;

#define DEV_NUN 2

    virtio_mmio_t virtio_devs[DEV_NUN] = {
        {.id = 0,
         .vm_id = 0,
         .va = VIRTIO_MMIO_ADDRESS,
         .pa = 0,
         .size = VIRTIO_MMIO_SIZE,
         .int_id = 0x10 + 32,
         .type = VIRTIO_TYPE_BLOCK},
        // {.id = 1,
        //  .vm_id = 1,
        //  .va = VIRTIO_MMIO_ADDRESS + VIRTIO_MMIO_SIZE,
        //  .pa = 2097152000,
        //  .size = VIRTIO_MMIO_SIZE,
        //  .int_id = 0x11 + 32,
        //  .type = VIRTIO_TYPE_BLOCK},
        {.id = 2,
         .vm_id = 0,
         .va = VIRTIO_MMIO_ADDRESS + VIRTIO_MMIO_SIZE * 2,
         .pa = 2097152000 * 2,
         .size = VIRTIO_MMIO_SIZE,
         .int_id = 0x12 + 32,
         .type = VIRTIO_TYPE_BLOCK}};

    for (int i = 0; i < DEV_NUN; i++) {
        virtio_mmio_manager.virt_mmio_devs[virtio_devs[i].id] = virtio_devs[i];
        virtio_mmio_manager.vm_has_dev |= (1 << virtio_devs[i].vm_id);
        virtio_mmio_manager.num++;
    }
    virtio_mmio_manager.devs_probed = true;
}

static bool virtio_mmio_init(virtio_mmio_t* virtio_mmio)
{
    virtio_mmio->regs.magic = VIRTIO_MMIO_MAGIG;
    virtio_mmio->regs.version = VIRTIO_VERSION;
    virtio_mmio->regs.vendor_id = VIRTIO_VENDER_ID;
    virtio_mmio->regs.device_id = virtio_mmio->type;
    virtio_mmio->regs.dev_feature = 0;
    virtio_mmio->regs.drv_feature = 0;
    virtio_mmio->regs.q_num_max = VIRTQUEUE_MAX_SIZE;

    if (!virt_dev_init(virtio_mmio)) {
        return false;
    }

    return true;
}

void virtio_init(vm_t* vm)
{
    spin_lock(&req_handler_lock);

    manager_add_virtio_mmio();

    if (!(virtio_mmio_manager.vm_has_dev & (1 << vm->id))) {
        INFO("vm%d has no virtio devs\n", vm->id);
        spin_unlock(&req_handler_lock);
        return;
    } else {
        INFO("vm%d has virtio devs", vm->id);
    }

    for (int i = 0; i < VIRTIO_MMIO_DEVICE_MAX; i++) {
        virtio_mmio_t* virtio_mmio = &virtio_mmio_manager.virt_mmio_devs[i];
        if (virtio_mmio->vm_id != vm->id || virtio_mmio->type == 0) continue;
        // FIXME: need to be a list
        vm->virtio = virtio_mmio;
        if (!virtio_mmio_init(virtio_mmio)) {
            ERROR("virt_dev init error!");
        }
        emul_region_t emu = {.va_base = virtio_mmio->va,
                             .pa_base = virtio_mmio->pa,
                             .size = virtio_mmio->size,
                             .handler = virtio_mmio->handler};
        vm_add_emul(vm, &emu);
    }

    INFO("vm%d virtio_init ok", vm->id);

    spin_unlock(&req_handler_lock);
}