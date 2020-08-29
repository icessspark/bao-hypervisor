#ifndef __VIRT_DEV_H__
#define __VIRT_DEV_H__

#include <bao.h>
#include <objcache.h>
#include <virt_blk.h>
#include <virtio_mmio.h>
#include <virtq.h>

/* VIRTIO_DEVICE_TYPE*/
#define VIRTIO_TYPE_NET 1
#define VIRTIO_TYPE_BLOCK 2
#define VIRTIO_TYPE_CONSOLE 3
#define VIRTIO_TYPE_ENTROPY 4
#define VIRTIO_TYPE_BALLOON 5
#define VIRTIO_TYPE_IOMEMORY 6
#define VIRTIO_TYPE_RPMSG 7
#define VIRTIO_TYPE_SCSI 8
#define VIRTIO_TYPE_9P 9
#define VIRTIO_TYPE_INPUT 18

typedef struct virtio_mmio virtio_mmio_t;
// typedef bool (*dev_handler_t)(/*void* req,  void* buffer*/);

struct virt_dev {
    bool activated;
    uint32_t vq_sel;
    uint32_t type;
    uint64_t features;
    uint64_t generation;

    objcache_t* desc_cache;
    void* desc;

    objcache_t* req_cache;
    void* req;
};

typedef struct virt_dev virt_dev_t;

bool virt_dev_init(virtio_mmio_t* virtio_mmio);
void virt_dev_reset(virtio_mmio_t* v_m);

bool virtio_be_handler(emul_access_t* acc);

#endif /* __VIRT_DEV_H__ */
