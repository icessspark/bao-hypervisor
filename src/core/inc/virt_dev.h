#ifndef __VIRT_DEV_H__
#define __VIRT_DEV_H__

#include <bao.h>
#include <objcache.h>
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

/* GENERAL_FEATURES */
#define VIRTIO_F_VERSION_1 1ULL << 32

/* VIRTIO_BLK_FEATURES*/
#define VIRTIO_BLK_F_SIZE_MAX 1ULL << 1
#define VIRTIO_BLK_F_SEG_MAX 1ULL << 2
#define VIRTIO_BLK_F_GEOMETR 1ULL << 4
#define VIRTIO_BLK_F_RO 1ULL << 5
#define VIRTIO_BLK_F_BLK_SIZE 1ULL << 6
#define VIRTIO_BLK_F_FLUSH 1ULL << 9
#define VIRTIO_BLK_F_TOPOLOGY 1ULL << 10
#define VIRTIO_BLK_F_CONFIG_WCE 1ULL << 11
#define VIRTIO_BLK_F_DISCARD \
    1ULL << 13  // Not support by virtio_blk driver in linux 4.19
#define VIRTIO_BLK_F_WRITE_ZEROES \
    1ULL << 14  // Not support by virtio_blk driver in linux 4.19

/* BLOCK PARAMETERS*/
#ifndef SECTOR_BSIZE
#define SECTOR_BSIZE 512
#endif
#define BLOCKIF_IOV_MAX 1 /* not practical to be IOV_MAX */

/* BLOCK REQUEST TYPE*/
#define VIRTIO_BLK_T_IN 0
#define VIRTIO_BLK_T_OUT 1
#define VIRTIO_BLK_T_FLUSH 4
#define VIRTIO_BLK_T_GET_ID 8
#define VIRTIO_BLK_T_DISCARD 11
#define VIRTIO_BLK_T_WRITE_ZEROES 13

/* BLOCK REQUEST STATUS*/
#define VIRTIO_BLK_S_OK 0
#define VIRTIO_BLK_S_IOERR 1
#define VIRTIO_BLK_S_UNSUPP 2

#define VIRTIO_BLK_ID_BYTES 20

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

struct virtio_blk_req {
    uint32_t type;
    uint32_t reserved;  // disk region offset
    uint64_t sector;
    uint64_t data_bg;
    uint32_t len;
    uint64_t* status;
};

bool virt_dev_init(virtio_mmio_t* virtio_mmio);
bool virtio_be_blk_handler(emul_access_t* acc);
void virt_dev_reset(virtio_mmio_t* v_m);

void blk_req_handler(void* req, void* buffer);

#endif /* __VIRT_DEV_H__ */
