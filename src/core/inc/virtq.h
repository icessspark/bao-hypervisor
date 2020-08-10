#ifndef __VIRT_Q_H__
#define __VIRT_Q_H__

#include <bao.h>
#include <list.h>
#include <util.h>
#include <objcache.h>
#include <virtio_mmio.h>
#include <virt_dev.h>

// virtq的最大容量
#define VIRTQUEUE_MAX_SIZE		(8)

#define VIRQ_STATE_INACTIVE		(0x0)
#define VIRQ_STATE_PENDING		(0x1)
#define VIRQ_STATE_ACTIVE		(0x2)
#define VIRQ_STATE_ACTIVE_AND_PENDING	(0x3)

#define VCPU_MAX_ACTIVE_VIRQS		(64)
#define VIRQ_INVALID_ID			(0xff)

#define VIRQ_ACTION_REMOVE	(0x0)
#define VIRQ_ACTION_ADD		(0x1)
#define VIRQ_ACTION_CLEAR	(0x2)

#define VIRQ_AFFINITY_VM_ANY	(0xffff)
#define VIRQ_AFFINITY_VCPU_ANY	(0xff)

#define CONFIG_NR_SGI_IRQS  16  /*软中断个数*/
#define CONFIG_NR_PPI_IRQS  16  /*私有外设中断个数*/
#define VM_SGI_VIRQ_NR		(CONFIG_NR_SGI_IRQS)
#define VM_PPI_VIRQ_NR		(CONFIG_NR_PPI_IRQS)
#define VM_LOCAL_VIRQ_NR	(VM_SGI_VIRQ_NR + VM_PPI_VIRQ_NR)

#ifndef CONFIG_HVM_SPI_VIRQ_NR
#define HVM_SPI_VIRQ_NR		(384)
#else
#define HVM_SPI_VIRQ_NR		CONFIG_HVM_SPI_VIRQ_NR
#endif

#define HVM_SPI_VIRQ_BASE	(VM_LOCAL_VIRQ_NR)

#define GVM_SPI_VIRQ_NR		(64)
#define GVM_SPI_VIRQ_BASE	(VM_LOCAL_VIRQ_NR)

#define VIRQ_SPI_OFFSET(virq)	((virq) - VM_LOCAL_VIRQ_NR)
#define VIRQ_SPI_NR(count)	((count) > VM_LOCAL_VIRQ_NR ? VIRQ_SPI_OFFSET((count)) : 0)

#define VM_VIRQ_NR(nr)		((nr) + VM_LOCAL_VIRQ_NR)

#define MAX_HVM_VIRQ		(HVM_SPI_VIRQ_NR + VM_LOCAL_VIRQ_NR)
#define MAX_GVM_VIRQ		(GVM_SPI_VIRQ_NR + VM_LOCAL_VIRQ_NR)

#define VIRQS_PENDING		(1 << 0)
#define VIRQS_ENABLED		(1 << 1)
#define VIRQS_SUSPEND		(1 << 2)
#define VIRQS_HW		(1 << 3)
#define VIRQS_CAN_WAKEUP	(1 << 4)
#define VIRQS_REQUESTED		(1 << 6)
#define VIRQS_FIQ		(1 << 7)

#define VIRQF_CAN_WAKEUP	(1 << 4)
#define VIRQF_ENABLE		(1 << 5)
#define VIRQF_FIQ		(1 << 7)

typedef struct virq_desc {
	uint8_t id;
	uint8_t state;
	uint8_t pr;
	uint8_t src;
	uint8_t type;
	uint8_t vcpu_id;
	uint16_t vmid;
	uint16_t vno;
	uint16_t hno;
	uint32_t flags;
	list_t list;
} virq_desc_t;

typedef struct virq {
	uint32_t active_count;
	uint32_t pending_hirq;
	uint32_t pending_virq;
	spinlock_t lock;
	list_t pending_list;
	list_t active_list;
	virq_desc_t local_desc[VM_LOCAL_VIRQ_NR];
// #if defined(CONFIG_VIRQCHIP_VGICV2) || defined(CONFIG_VIRQCHIP_VGICV3)
// #define MAX_NR_LRS 64
// 	struct ffs_table lrs_table;
// #endif
} virq_t;

struct vring_desc {
    /*Address (guest-physical)*/
    uint64_t addr;
    /* Length */
    uint32_t len;
    /* The flags as indicated above */
    uint16_t flags;
    /* We chain unused descriptors via this, too */
    uint16_t next;
};

struct vring_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[];
};

struct vring_used_elem {
    // Index of start of used descriptor chain 
    uint32_t id;
    // Total length of the descriptor chain which was used (written to) 
    uint32_t len;
};

struct vring_used {
    uint16_t flags;
    uint16_t idx;
    struct vring_used_elem ring[];
};

static void inline virq_set_enable(virq_desc_t *d)
{
	d->flags |= VIRQS_ENABLED;
}

static void inline virq_clear_enable(virq_desc_t *d)
{
	d->flags &= ~VIRQS_ENABLED;
}

static int inline virq_is_enabled(virq_desc_t *d)
{
	return (d->flags & VIRQS_ENABLED);
}

static void inline virq_set_wakeup(virq_desc_t *d)
{
	d->flags |= VIRQS_CAN_WAKEUP;
}

static void inline virq_clear_wakeup(virq_desc_t *d)
{
	d->flags &= ~VIRQS_CAN_WAKEUP;
}

static int inline virq_can_wakeup(virq_desc_t *d)
{
	return (d->flags & VIRQS_CAN_WAKEUP);
}

static void inline virq_set_suspend(virq_desc_t *d)
{
	d->flags |= VIRQS_SUSPEND;
}

static void inline virq_clear_suspend(virq_desc_t *d)
{
	d->flags &= ~VIRQS_SUSPEND;
}

static int inline virq_is_suspend(virq_desc_t *d)
{
	return (d->flags & VIRQS_SUSPEND);
}

static void inline virq_set_hw(virq_desc_t *d)
{
	d->flags |= VIRQS_HW;
}

static void inline virq_clear_hw(virq_desc_t *d)
{
	d->flags &= ~VIRQS_HW;
}

static int inline virq_is_hw(virq_desc_t *d)
{
	return (d->flags & VIRQS_HW);
}

static void inline virq_set_pending(virq_desc_t *d)
{
	d->flags |= VIRQS_PENDING;
}

static void inline virq_clear_pending(virq_desc_t *d)
{
	d->flags &= ~VIRQS_PENDING;
}

static int inline virq_is_pending(virq_desc_t *d)
{
	return (d->flags & VIRQS_PENDING);
}

static int inline virq_is_requested(virq_desc_t *d)
{
	return (d->flags & VIRQS_REQUESTED);
}

static void inline __virq_set_fiq(virq_desc_t *d)
{
	d->flags |= VIRQS_FIQ;
}

static int inline virq_is_fiq(virq_desc_t *d)
{
	return (d->flags & VIRQS_FIQ);
}

static void inline virq_clear_fiq(virq_desc_t *d)
{
	d->flags &= ~VIRQS_FIQ;
}

int virq_enable(vcpu_t *vcpu, uint32_t virq);
int virq_disable(vcpu_t *vcpu, uint32_t virq);
void virq_init(vcpu_t *vcpu);
void virq_reset(vcpu_t *vcpu);




#endif /* __VIRT_DEV_H__ */


