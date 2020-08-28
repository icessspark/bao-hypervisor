#include <bao.h>
#include <sched.h>
#include <tlb.h>

extern uint8_t _image_start, _image_end, _cpu_private_beg, _cpu_private_end,
    _vm_beg, _vm_end;

void vcpu_pool_init()
{
    vcpu_pool.num = 0;
    for (int i = 0; i < VCPU_POOL_MAX; i++) {
        vcpu_pool.vcpu_content[i].active = false;
    }
}

bool vcpu_pool_append(vcpu_t* vcpu, pte_t pt)
{
    if (vcpu_pool.num == VCPU_POOL_MAX) {
        WARNING("Can't generate more vcpu!");
        return false;
    }

    vcpu_pool.vcpu_content[vcpu_pool.num].vcpu = vcpu;
    vcpu_pool.vcpu_content[vcpu_pool.num].pt = pt;

    vcpu_pool.num++;

    return true;
}

int vcpu_pool_get_pending_idx()
{
    for (int idx = 0; idx < vcpu_pool.num; idx++) {
        if (!vcpu_pool.vcpu_content[idx].active) {
            return idx;
        }
    }

    return -1;
}

void vcpu_pool_switch(int vcpu_id)
{
    tlb_vm_inv_all(cpu.vcpu->vm->id);
    cpu.vcpu->active = false;

    uint64_t elr = cpu.vcpu->regs->elr_el2;
    printf("[vm%d] elr 0x%lx\n", cpu.vcpu->vm->id, elr);

    int pending_idx = -1;
    if (vcpu_id < 0) {
        pending_idx = vcpu_pool_get_pending_idx();
        if (pending_idx == -1) {
            WARNING("No pending vcpu!");
            return;
        }
    } else {
        pending_idx = vcpu_id;
    }

    vcpu_pool.vcpu_content[active_idx].active = false;
    active_idx = pending_idx;
    vcpu_content_t* pending_vcpu = &vcpu_pool.vcpu_content[pending_idx];
    pending_vcpu->active = true;

    printf("switch vm %d to vm %d\n", cpu.vcpu->vm->id,
           pending_vcpu->vcpu->vm->id);

    cpu.vcpu = pending_vcpu->vcpu;
    vm_t* vm = cpu.vcpu->vm;

    MSR(VMPIDR_EL2, cpu.vcpu->arch.vmpidr);

    uint64_t root_pt_pa;
    mem_translate(&cpu.as, vm->as.pt.root, &root_pt_pa);
    MSR(VTTBR_EL2, ((vm->id << VTTBR_VMID_OFF) & VTTBR_VMID_MSK) |
                       (root_pt_pa & ~VTTBR_VMID_MSK));

    pte_t* pte = cpu.as.pt.root + PT_VM_REC_IND;
    pte_set(pte, root_pt_pa, PTE_TABLE, PTE_HYP_FLAGS);

    ISB();  // make sure vmid is commited befor tlb
    tlb_vm_inv_all(vm->id);
    tlb_hyp_inv_all();

    size_t image_size = (size_t)(&_image_end - &_image_start);
    size_t vm_size = (size_t)(&_vm_end - &_vm_beg); 

    cache_flush_range(&_image_start, image_size);
    cache_flush_range(&_cpu_private_beg, sizeof(cpu_t));

    cpu.vcpu->active = true;
}