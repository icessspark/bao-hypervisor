/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2018, Tuomas Tynkkynen <tuomas.tynkkynen@iki.fi>
 * Copyright (C) 2018, Bin Meng <bmeng.cn@gmail.com>
 *
 * VirtIO is a virtualization standard for network and disk device drivers
 * where just the guest's device driver "knows" it is running in a virtual
 * environment, and cooperates with the hypervisor. This enables guests to
 * get high performance network and disk operations, and gives most of the
 * performance benefits of paravirtualization. In the U-Boot case, the guest
 * is U-Boot itself, while the virtual environment are normally QEMU targets
 * like ARM, RISC-V and x86.
 *
 * See http://docs.oasis-open.org/virtio/virtio/v1.0/virtio-v1.0.pdf for
 * the VirtIO specification v1.0.
 *
 * This file is largely based on Linux kernel virtio_*.h files
 */

#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include "types.h"

#define VIRTIO_ID_NET		1 /* virtio net */
#define VIRTIO_ID_BLOCK		2 /* virtio block */
#define VIRTIO_ID_RNG		4 /* virtio rng */
#define VIRTIO_ID_MAX_NUM	5

#define VIRTIO_NET_DRV_NAME	"virtio-net"
#define VIRTIO_BLK_DRV_NAME	"virtio-blk"
#define VIRTIO_RNG_DRV_NAME	"virtio-rng"

/* Status byte for guest to report progress, and synchronize features */

/* We have seen device and processed generic fields (VIRTIO_CONFIG_F_VIRTIO) */
#define VIRTIO_CONFIG_S_ACKNOWLEDGE	1
/* We have found a driver for the device */
#define VIRTIO_CONFIG_S_DRIVER		2
/* Driver has used its parts of the config, and is happy */
#define VIRTIO_CONFIG_S_DRIVER_OK	4
/* Driver has finished configuring features */
#define VIRTIO_CONFIG_S_FEATURES_OK	8
/* Device entered invalid state, driver must reset it */
#define VIRTIO_CONFIG_S_NEEDS_RESET	0x40
/* We've given up on this device */
#define VIRTIO_CONFIG_S_FAILED		0x80

/*
 * Virtio feature bits VIRTIO_TRANSPORT_F_START through VIRTIO_TRANSPORT_F_END
 * are reserved for the transport being used (eg: virtio_ring, virtio_pci etc.),
 * the rest are per-device feature bits.
 */
#define VIRTIO_TRANSPORT_F_START	28
#define VIRTIO_TRANSPORT_F_END		38

#ifndef VIRTIO_CONFIG_NO_LEGACY
/*
 * Do we get callbacks when the ring is completely used,
 * even if we've suppressed them?
 */
#define VIRTIO_F_NOTIFY_ON_EMPTY	24

/* Can the device handle any descriptor layout? */
#define VIRTIO_F_ANY_LAYOUT		27
#endif /* VIRTIO_CONFIG_NO_LEGACY */

/* v1.0 compliant */
#define VIRTIO_F_VERSION_1		32

/*
 * If clear - device has the IOMMU bypass quirk feature.
 * If set - use platform tools to detect the IOMMU.
 *
 * Note the reverse polarity (compared to most other features),
 * this is for compatibility with legacy systems.
 */
#define VIRTIO_F_IOMMU_PLATFORM		33

/* Does the device support Single Root I/O Virtualization? */
#define VIRTIO_F_SR_IOV			37


#endif