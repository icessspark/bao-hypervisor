#ifndef __SECHED_H__
#define __SECHED_H__

#include <cpu.h>
#include <vm.h>

#define VCPU_POOL_MAX 64
#define ANY_PENDING_VCPU -1

volatile uint64_t active_idx;

typedef struct vcpu_content {
    vcpu_t* vcpu;
    pte_t pt;
    bool active;
} vcpu_content_t;

volatile struct vcpu_pool_t {
    vcpu_content_t vcpu_content[VCPU_POOL_MAX];
    uint64_t num;
} vcpu_pool;

void vcpu_pool_init();
bool vcpu_pool_append(vcpu_t* vcpu, pte_t pt);
bool vcpu_pool_remove(uint64_t index);
int vcpu_pool_get_pending_idx();
void vcpu_pool_switch(int vcpu_id);

#endif /* __SECHED_H__ */
