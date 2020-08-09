//
// Created by tonny on 8/7/20.
//

#ifndef BAO_VIRTIO_PRELUDE_H
#define BAO_VIRTIO_PRELUDE_H

void virtio_blk_init();

void virtio_blk_read(unsigned long sector, unsigned long count, void *buf);

void virtio_blk_write(unsigned long sector, unsigned long count, const void *buf);

void virtio_blk_handler();

#endif  // BAO_VIRTIO_PRELUDE_H
