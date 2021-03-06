#ifndef __VIRT_Q_H__
#define __VIRT_Q_H__

#include <bao.h>
#include <list.h>
#include <objcache.h>
#include <util.h>
#include <virt_dev.h>
#include <virtio_mmio.h>
#include <arch/fences.h>

typedef struct virtio_mmio virtio_mmio_t;

// virtq的最大容量
// FIXME: 1024 would break
#define VIRTQUEUE_MAX_SIZE (64)

/* This marks a buffer as continuing via the next field. */
#define VIRTQ_DESC_F_NEXT 1
/* This marks a buffer as write-only (otherwise read-only). */
#define VIRTQ_DESC_F_WRITE 2
/* This means the buffer contains a list of buffer descriptors. */
#define VIRTQ_DESC_F_INDIRECT 4

#define VRING_ALIGN_SIZE (4096)

#define VRING_AVAIL_F_NO_INTERRUPT 1
#define VRING_USED_F_NO_NOTIFY 1

#define VIRTIO_RING_F_INDIRECT_DESC 28
#define VIRTIO_RING_F_EVENT_IDX 29

#define VIRTIO_F_NOTIFY_ON_EMPTY 24
#define VIRTIO_F_ANY_LAYOUT 27

#define VRING_AVAIL_ALIGN_SIZE 2
#define VRING_USED_ALIGN_SIZE 4
#define VRING_DESC_ALIGN_SIZE 16

struct virtq {
    int ready;
    uint16_t vq_index;
    unsigned int num;
    // unsigned int iovec_size;
    struct vring_desc volatile *desc;
    struct vring_avail volatile *avail;
    struct vring_used volatile *used;
    uint16_t last_avail_idx;
    uint16_t last_used_idx;
    uint16_t used_flags;
    bool to_notify;

    bool (*notify_handler)(struct virtq *, struct virtio_mmio *);
};

typedef struct virtq virtq_t;

// static inline uint32_t vring_size(unsigned int qsz)
// {
//     return ALIGN((sizeof(struct vring_desc) * qsz) +
//                      (sizeof(uint16_t) * (2 + qsz)),
//                  VRING_ALIGN_SIZE) +
//            ALIGN(sizeof(struct vring_used_elem) * qsz, VRING_ALIGN_SIZE);
// }

void virtq_init(virtq_t *vq);
bool process_guest_blk_notify(virtq_t *, virtio_mmio_t *);



#endif /* __VIRT_DEV_H__ */
