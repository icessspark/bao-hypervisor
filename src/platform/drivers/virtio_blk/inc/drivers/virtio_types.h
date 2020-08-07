/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (C) 2018, Tuomas Tynkkynen <tuomas.tynkkynen@iki.fi>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * From Linux kernel include/uapi/linux/virtio_types.h
 */

#ifndef _LINUX_VIRTIO_TYPES_H
#define _LINUX_VIRTIO_TYPES_H

#include "types.h"

/*
 * __virtio{16,32,64} have the following meaning:
 * - __u{16,32,64} for virtio devices in legacy mode, accessed in native endian
 * - __le{16,32,64} for standard-compliant virtio devices
 */

typedef u16 __virtio16;
typedef u32 __virtio32;
typedef u64 __virtio64;

#endif /* _LINUX_VIRTIO_TYPES_H */
