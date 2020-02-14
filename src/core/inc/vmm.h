/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __VMM_H__
#define __VMM_H__

#include <bao.h>
#include <vm.h>
#include <list.h>

typedef struct ipc_info{
    node_t node;

    ipc_obj_t ipc_obj;
    list_t vms;
}ipc_info_t;

struct vm_public_node{
    node_t node;
    vm_public_t *vm_public;
};

extern objcache_t vm_node_oc;
extern objcache_t vm_pub_oc;

void vmm_init();
void vmm_arch_init();

void add_vm_public(struct vm_public_node *vm_public);

ipc_info_t* find_ipc_obj_in_list(ipc_obj_t *ipc_obj);
ipc_info_t* create_ipc_node();


#endif /* __VMM_H__ */
