#ifndef __IPC_H__
#define __IPC_H__

#include <mem.h>

#define HVC_IS_IPC(fid) (fid & 0xffff)

/* interface */
typedef struct ipc_if{
    int irq_num;
}ipc_if_t;

/* identify ipc, and core data */
typedef struct ipc_obj{
    int id;
    shmem_t *shmem;
}ipc_obj_t;

/* data for config */
typedef struct ipc_obj_config {
    ipc_obj_t ipc_obj;
    ipc_if_t ipc_if;
}ipc_obj_config_t;

int handle_ipc_call(uint64_t fid, uint64_t x1, uint64_t x2, uint64_t x3);

#endif
