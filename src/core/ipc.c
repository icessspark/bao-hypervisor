#include <ipc.h>
#include <cpu.h>
#include <vmm.h>

enum {IPC_NOTIFY};

extern cpu_t cpu;
extern list_t ipc_obj_list;

static void ipc_handler(uint32_t event, uint64_t data){
    int irq = -1;
    switch(event){
    case IPC_NOTIFY:
        /* send the SGI, assume only one VM per core */
        irq = data;
        interrupts_vm_inject(cpu.vcpu->vm, irq, -1/*anonymous source*/);
    break;
    }
}
CPU_MSG_HANDLER(ipc_handler, IPC_CPUSMG_ID);

static void notify_ipc_participants(ipc_info_t *ipc_info)
{
    list_foreach(ipc_info->vms, struct vm_public_node, it){
        if(it->vm_public->id == cpu.vcpu->vm->id)
            continue;

        int irqnum = it->vm_public->ipc_if.irq_num;
        cpu_msg_t msg = {IPC_CPUSMG_ID, IPC_NOTIFY, irqnum};

        int dest_cpuid = -1;
        for(int cpu_id = 0; cpu_id < platform.cpu_num; cpu_id++){
            if(((1U << cpu_id) & it->vm_public->cpus) && (cpu_id != cpu.id)){
                /* we've found a cpu id */
                dest_cpuid = cpu_id;
                break;
            }
        }
        if(dest_cpuid == -1){
            ERROR("Could not find destination ipc participant phys cpu");
        }
        cpu_send_msg(dest_cpuid, &msg);
    }
}


static ipc_info_t* find_guest_ipc_by_id(uint64_t id)
{
    /* search all ipc_objects, for the ipc id */
    /* check if the vm participates in the selected ipc */
    list_foreach(ipc_obj_list, struct ipc_info, ipc_it){
        if(ipc_it->ipc_obj.id == id){
            list_foreach(ipc_it->vms, struct vm_public_node, vm_it){
                if(vm_it->vm_public->id == cpu.vcpu->vm->id){
                    return ipc_it;
                }
            }
            /* We've found the ID but vm does not have access to it */
            return NULL;
        }
    }
    return NULL;
}

int handle_ipc_call(uint64_t fid, uint64_t ipc_id, uint64_t x2, uint64_t x3)
{
    /* get IPC */
    ipc_info_t *ipc = find_guest_ipc_by_id(ipc_id);
    if(!ipc)
        return -1;

    notify_ipc_participants(ipc);

    return 0;
}
