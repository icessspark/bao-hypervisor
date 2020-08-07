//
// Created by tonny on 8/7/20.
//

#ifndef BAO_VIRTIO_PRELUDE_H
#define BAO_VIRTIO_PRELUDE_H

void virtio_blk_init();

void virtio_blk_read(unsigned long start, unsigned long blkcnt, void *buffer);

void virtio_blk_write(unsigned long start, unsigned long blkcnt, const void *buffer);

#endif  // BAO_VIRTIO_PRELUDE_H
