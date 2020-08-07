/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2018, Tuomas Tynkkynen <tuomas.tynkkynen@iki.fi>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * From Linux kernel include/uapi/linux/virtio_ring.h
 */

#ifndef _LINUX_VIRTIO_RING_H
#define _LINUX_VIRTIO_RING_H

#include <drivers/virtio_types.h>

/* This marks a buffer as continuing via the next field */
#define VRING_DESC_F_NEXT		1
/* This marks a buffer as write-only (otherwise read-only) */
#define VRING_DESC_F_WRITE		2
/* This means the buffer contains a list of buffer descriptors */
#define VRING_DESC_F_INDIRECT		4

/*
 * The Host uses this in used->flags to advise the Guest: don't kick me when
 * you add a buffer. It's unreliable, so it's simply an optimization. Guest
 * will still kick if it's out of buffers.
 */
#define VRING_USED_F_NO_NOTIFY		1

/*
 * The Guest uses this in avail->flags to advise the Host: don't interrupt me
 * when you consume a buffer. It's unreliable, so it's simply an optimization.
 */
#define VRING_AVAIL_F_NO_INTERRUPT	1

/* We support indirect buffer descriptors */
#define VIRTIO_RING_F_INDIRECT_DESC	28

/*
 * The Guest publishes the used index for which it expects an interrupt
 * at the end of the avail ring. Host should ignore the avail->flags field.
 *
 * The Host publishes the avail index for which it expects a kick
 * at the end of the used ring. Guest should ignore the used->flags field.
 */
#define VIRTIO_RING_F_EVENT_IDX		29

/* Virtio ring descriptors: 16 bytes. These can chain together via "next". */
struct vring_desc {
    /* Address (guest-physical) */
    __virtio64 addr;
    /* Length */
    __virtio32 len;
    /* The flags as indicated above */
    __virtio16 flags;
    /* We chain unused descriptors via this, too */
    __virtio16 next;
};

struct vring_avail {
    __virtio16 flags;
    __virtio16 idx;
    __virtio16 ring[];
};

struct vring_used_elem {
    /* Index of start of used descriptor chain */
    __virtio32 id;
    /* Total length of the descriptor chain which was used (written to) */
    __virtio32 len;
};

struct vring_used {
    __virtio16 flags;
    __virtio16 idx;
    struct vring_used_elem ring[];
};


#endif /* _LINUX_VIRTIO_RING_H */
