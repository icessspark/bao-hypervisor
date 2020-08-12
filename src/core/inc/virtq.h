#ifndef __VIRT_Q_H__
#define __VIRT_Q_H__

#include <bao.h>
#include <list.h>
#include <util.h>
#include <objcache.h>
#include <virtio_mmio.h>
#include <virt_dev.h>

typedef struct virtio_mmio virtio_mmio_t;

// virtq的最大容量
#define VIRTQUEUE_MAX_SIZE		(8)

/* This marks a buffer as continuing via the next field. */ 
#define VIRTQ_DESC_F_NEXT 1 
/* This marks a buffer as write-only (otherwise read-only). */ 
#define VIRTQ_DESC_F_WRITE 2 
/* This means the buffer contains a list of buffer descriptors. */ 
#define VIRTQ_DESC_F_INDIRECT 4

#define VRING_ALIGN_SIZE		(4096)

#define VRING_AVAIL_F_NO_INTERRUPT	1
#define VRING_USED_F_NO_NOTIFY		1

#define VIRTIO_RING_F_INDIRECT_DESC	28
#define VIRTIO_RING_F_EVENT_IDX		29

#define VIRTIO_F_NOTIFY_ON_EMPTY	24
#define VIRTIO_F_ANY_LAYOUT		27

#define VRING_AVAIL_ALIGN_SIZE		2
#define VRING_USED_ALIGN_SIZE		4
#define VRING_DESC_ALIGN_SIZE		16

struct virtq {
    int ready;
	unsigned int num;
	unsigned int iovec_size;
	struct vring_desc *desc;
	struct vring_avail *avail;
	struct vring_used *used;
	uint16_t last_avail_idx;
	uint16_t avail_idx;
	uint16_t last_used_idx;
	// uint16_t used_flags;
	uint16_t vq_index;

	void (*notify_handler)(struct virtq *, struct virtio_mmio *);

};

typedef struct virtq virtq_t;

struct vring_desc {
    /*Address (guest-physical)*/
    uint64_t addr;
    /* Length */
    uint32_t len;
    /* The flags as indicated above */
    uint16_t flags;
    /* We chain unused descriptors via this, too */
    uint16_t next;
}__attribute__((__packed__));


struct vring_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
    // uint16_t used_event; /* Only if VIRTIO_F_EVENT_IDX */
}__attribute__((__packed__));


struct vring_used_elem {
    // Index of start of used descriptor chain
    uint32_t id;
    // Total length of the descriptor chain which was used (written to) 
    uint32_t len;
}__attribute__((__packed__));

struct vring_used {
    uint16_t flags;
    uint16_t idx;
    struct vring_used_elem ring[];
    // uint16_t avail_event; /* Only if VIRTIO_F_EVENT_IDX */
}__attribute__((__packed__));

/* Structure for scatter/gather I/O.  */
struct iovec {
    void *iov_base;	/* Pointer to data.  */
    uint32_t iov_len;	/* Length of data.  */
};

struct virtq_ops {
	int (*vq_init)(virtq_t *);
	int (*vq_reset)(virtq_t *);
	void (*vq_deinit)(virtq_t *);
	void (*neg_features)(virtq_t *);
};

#define virtq_used_event(vq) \
	(uint16_t *)&vq->avail->ring[vq->num]
#define virtq_avail_event(vq) \
	(uint16_t *)&vq->used->ring[vq->num]

static int inline virtq_has_descs(virtq_t *vq)
{
	return vq->avail->idx != vq->last_avail_idx;
}

static inline uint32_t vring_size (unsigned int qsz)
{
	return ALIGN((sizeof(struct vring_desc) * qsz) +
		 (sizeof(uint16_t) * (2 + qsz)), VRING_ALIGN_SIZE) +
	       ALIGN(sizeof(struct vring_used_elem) * qsz, VRING_ALIGN_SIZE);
}

// static int inline virtq_has_feature(virtq_t *vq, uint32_t fe)
// {
// 	return !!(vq->dev->master->driver_features & (1UL << fe));
// }

// static inline void virtio_send_irq(virt_dev_t *dev, int type)
// {
// 	uint32_t value = 0;
    
// 	value = dev->master->regs.irt_stat;
// 	// rmb();

// 	value |= type;
//     dev->master->regs.irt_stat = value;
// 	// wmb();

// 	// vdev_send_irq(dev->vdev);
// }


int virtq_notify(virtq_t *virtq);
int virtq_disable_notify(virtq_t *virtq);
int virtq_enable_notify(virtq_t *virtq);

void virtq_init(virtq_t *virtq);

void process_guest_blk_notify(virtq_t *, virtio_mmio_t *);


#endif /* __VIRT_DEV_H__ */


