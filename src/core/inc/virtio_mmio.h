#ifndef __VIRTIO_MMIO_H__
#define __VIRTIO_MMIO_H__

#include <emul.h>
#include <objcache.h>
#include <virt_dev.h>
#include <virtq.h>
#include <vm.h>

#define VIRTIO_MMIO_ADDRESS 0xa000000
#define VIRTIO_MMIO_SIZE 0x200
#define VIRTIO_MMIO_DEVICE_MAX 64

#define VIRTIO_MMIO_MAGIG (0x74726976)
#define VIRTIO_VENDER_ID (0x8888)
#define VIRTIO_VERSION (0x2)

#define VIRTIO_DEVEICE_FEATURE_MAX 32
#define VIRTIO_DRIVER_FEATURE_MAX 32

/*
 * for detail of the below mmio register, refer
 * to the Documents/virtio-v1.1.pdf
 */
#define VIRTIO_MMIO_MAGIC_VALUE 0x000
#define VIRTIO_MMIO_VERSION 0x004
#define VIRTIO_MMIO_DEVICE_ID 0x008
#define VIRTIO_MMIO_VENDOR_ID 0x00c
#define VIRTIO_MMIO_HOST_FEATURES 0x010
#define VIRTIO_MMIO_HOST_FEATURES_SEL 0x014
#define VIRTIO_MMIO_GUEST_FEATURES 0x020
#define VIRTIO_MMIO_GUEST_FEATURES_SEL 0x024
#define VIRTIO_MMIO_GUEST_PAGE_SIZE 0x028
#define VIRTIO_MMIO_QUEUE_SEL 0x030
#define VIRTIO_MMIO_QUEUE_NUM_MAX 0x034
#define VIRTIO_MMIO_QUEUE_NUM 0x038
#define VIRTIO_MMIO_QUEUE_ALIGN 0x03c
#define VIRTIO_MMIO_QUEUE_READY 0x044
#define VIRTIO_MMIO_QUEUE_PFN 0x040
#define VIRTIO_MMIO_QUEUE_NOTIFY 0x050
#define VIRTIO_MMIO_INTERRUPT_STATUS 0x060
#define VIRTIO_MMIO_INTERRUPT_ACK 0x064
#define VIRTIO_MMIO_STATUS 0x070
#define VIRTIO_MMIO_QUEUE_DESC_LOW 0x080
#define VIRTIO_MMIO_QUEUE_DESC_HIGH 0x084
#define VIRTIO_MMIO_QUEUE_AVAIL_LOW 0x090
#define VIRTIO_MMIO_QUEUE_AVAIL_HIGH 0x094
#define VIRTIO_MMIO_QUEUE_USED_LOW 0x0a0
#define VIRTIO_MMIO_QUEUE_USED_HIGH 0x0a4
#define VIRTIO_MMIO_CONFIG_GENERATION 0x0fc
#define VIRTIO_MMIO_CONFIG 0x100
#define VIRTIO_MMIO_REGS_END 0x200

/* below register from 0x300 is used for hypervisor */
#define VIRTIO_MMIO_GVM_ADDR 0x300
#define VIRTIO_MMIO_DEV_FLAGS 0x304
#define VIRTIO_MMIO_HOST_FEATURE0 0x308
#define VIRTIO_MMIO_HOST_FEATURE1 0x30c
#define VIRTIO_MMIO_HOST_FEATURE2 0x310
#define VIRTIO_MMIO_HOST_FEATURE3 0x314
#define VIRTIO_MMIO_DRIVER_FEATURE0 0x318
#define VIRTIO_MMIO_DRIVER_FEATURE1 0x31c
#define VIRTIO_MMIO_DRIVER_FEATURE2 0x320
#define VIRTIO_MMIO_DRIVER_FEATURE3 0x324

#define VIRTIO_MMIO_INT_VRING (1 << 0)
#define VIRTIO_MMIO_INT_CONFIG (1 << 1)

#define VIRTIO_EVENT_BUFFER_READY (1 << 0)
#define VIRTIO_EVENT_QUEUE_READY (1 << 1)
#define VIRTIO_EVENT_STATUS_CHANGE (1 << 2)
#define VIRTIO_EVENT_MMIO (1 << 3)

#define VIRTIO_DEV_STATUS_ACK (1)
#define VIRTIO_DEV_STATUS_DRIVER (2)
#define VIRTIO_DEV_STATUS_OK (4)
#define VIRTIO_DEV_STATUS_FEATURES_OK (8)
#define VIRTIO_DEV_NEEDS_RESET (64)
#define VIRTIO_DEV_STATUS_FAILED (128)

#define VIRTIO_MAX_FEATURE_SIZE (4)

#define VIRTIO_FEATURE_OFFSET(index) (VIRTIO_MMIO_HOST_FEATURE0 + index * 4)

typedef bool (*handler_t)(emul_access_t* acc);

typedef struct virt_mmio_regs {
    uint32_t magic;            // 0x000
    uint32_t version;          // 0x004
    uint32_t device_id;        // 0x008
    uint32_t vendor_id;        // 0x00c
    uint32_t dev_feature;      // 0x010
    uint32_t dev_feature_sel;  // 0x014
    uint32_t drv_feature;      // 0x020
    uint32_t drv_feature_sel;  // 0x024
    uint32_t q_sel;            // 0x030
    uint32_t q_num_max;        // 0x034
    uint32_t q_ready;          // 0x044
    uint32_t q_notify;         // 0x050
    uint32_t irt_stat;         // 0x060
    uint32_t irt_ack;          // 0x064
    uint32_t dev_stat;         // 0x070
    uint32_t q_desc_l;         // 0x080
    uint32_t q_desc_h;         // 0x084
    uint32_t q_drv_l;          // 0x090
    uint32_t q_drv_h;          // 0x094
    uint32_t q_dev_l;          // 0x0a0
    uint32_t q_dev_h;          // 0x0a4
} virt_mmio_regs_t;

struct virtio_mmio {
    uint32_t id;
    uint32_t vm_id;
    uint64_t va;
    uint64_t pa;
    uint64_t size;
    uint32_t type;
    handler_t handler;

    uint64_t driver_features;
    uint32_t driver_status;
    virt_mmio_regs_t regs;

    objcache_t* dev_cache;
    struct virt_dev* dev;

    objcache_t* vq_cache;
    struct virtq* vq;
};

typedef struct virtio_mmio virtio_mmio_t;

typedef struct virtio_mmio_manager {
    virtio_mmio_t virt_mmio_devs[VIRTIO_MMIO_DEVICE_MAX];
    bool devs_probed;
    uint16_t vm_has_dev;
    uint32_t num;
} virtio_mmio_manager_t;

extern virtio_mmio_manager_t virtio_mmio_manager;

void virtio_init(vm_t* vm);

static inline virtio_mmio_t* get_virt_mmio(uint64_t addr)
{
    return &virtio_mmio_manager.virt_mmio_devs[(addr - VIRTIO_MMIO_ADDRESS) /
                                               VIRTIO_MMIO_SIZE];
}

#endif /* __VIRTIO_MMIO_H__ */