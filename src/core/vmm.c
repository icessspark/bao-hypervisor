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

#include <config.h>
#include <cpu.h>
#include <fences.h>
#include <iommu.h>
#include <sched.h>
#include <spinlock.h>
#include <string.h>
#include <tlb.h>
#include <vm.h>
#include <vmm.h>

struct config* vm_config_ptr;

void vmm_init()
{
    vmm_arch_init();

    vcpu_pool_init();

    volatile static struct vm_assignment {
        bool master;
        size_t ncpus;
        pte_t vm_shared_table;
    } * vm_assign;

    size_t vmass_npages = 0;
    bool assigned = false;

    if (cpu.id == CPU_MASTER) {
        // iommu_init();

        vmass_npages =
            ALIGN(sizeof(struct vm_assignment) * vm_config_ptr->vmlist_size,
                  PAGE_SIZE) /
            PAGE_SIZE;
        vm_assign = mem_alloc_page(vmass_npages, SEC_HYP_GLOBAL, false);
        if (vm_assign == NULL) ERROR("cant allocate vm assignemnt pages");
        memset((void*)vm_assign, 0, vmass_npages * PAGE_SIZE);

        for (int vm_id = 0; vm_id < vm_config_ptr->vmlist_size; vm_id++) {
            while (vm_assign[vm_id].ncpus <
                   vm_config_ptr->vmlist[vm_id].platform.cpu_num) {
                if (!vm_assign[vm_id].master) {
                    vm_assign[vm_id].master = true;

                    size_t vm_npages = NUM_PAGES(sizeof(vm_t));
                    void* va = mem_alloc_vpage(
                        &cpu.as, SEC_HYP_VM,
                        (void*)(BAO_VM_BASE + 0x100000 * vm_id), vm_npages);
                    if (va != BAO_VM_BASE + 0x100000 * vm_id) {
                        ERROR("Unable to allocate vpages for vm!");
                    } else {
                        printf("[vmm_init] vm %d va 0x%lx \n", vm_id, va);
                    }
                    mem_map(&cpu.as, va, NULL, vm_npages, PTE_HYP_FLAGS);
                    memset(va, 0, vm_npages * PAGE_SIZE);

                    fence_ord_write();

                    vm_assign[vm_id].vm_shared_table =
                        *pt_get_pte(&cpu.as.pt, 0, va);

                    vm_init(va, &vm_config_ptr->vmlist[vm_id], true, vm_id,
                            vm_assign[vm_id].ncpus,
                            vm_assign[vm_id].vm_shared_table);
                } else {
                    // TODO: support multi vcpu
                }
                vm_assign[vm_id].ncpus++;
            }
        }


        active_idx = vcpu_pool_get_pending_idx();
        if (active_idx == -1) {
            ERROR("No pending vcpu!");
        }

        vcpu_pool.vcpu_content[active_idx].active = true;
        cpu.vcpu = vcpu_pool.vcpu_content[active_idx].vcpu;

        printf("Init vm_id %d\n", cpu.vcpu->vm->id);

        MSR(VMPIDR_EL2, cpu.vcpu->arch.vmpidr);

        uint64_t root_pt_pa;
        mem_translate(&cpu.as, cpu.vcpu->vm->as.pt.root, &root_pt_pa);
        MSR(VTTBR_EL2, ((cpu.vcpu->vm->id << VTTBR_VMID_OFF) &
        VTTBR_VMID_MSK) |
                           (root_pt_pa & ~VTTBR_VMID_MSK));

        pte_t* pte = cpu.as.pt.root + PT_VM_REC_IND;
        pte_set(pte, root_pt_pa, PTE_TABLE, PTE_HYP_FLAGS);

        ISB();  // make sure vmid is commited befor tlb
        tlb_vm_inv_all(cpu.vcpu->vm->id);

        assigned = true;
    }

    cpu_sync_barrier(&cpu_glb_sync);

    if (cpu.id == CPU_MASTER) {
        mem_free_vpage(&cpu.as, (void*)vm_assign, vmass_npages, true);
    }

    if (assigned) {
        printf("vcpu %d start running\n", cpu.vcpu->id);
        vcpu_run(cpu.vcpu);
    } else {
        cpu_idle();
    }
}
