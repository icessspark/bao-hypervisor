#include <bao.h>
#include <cpu.h>
#include <interrupts.h>
#include <spinlock.h>
#include <timer.h>

spinlock_t timer_handler_lock = SPINLOCK_INITVAL;

void timer_init()
{
    // interrupts_arch_cpu_enable(false);
    arch_timer_init(cpu.id);

    if (cpu.id == CPU_MASTER) {
        interrupts_reserve(NONSEC_EL2_PHY_TIMER_INT, timer_handler);
    }

    INFO("CPU%d timer_init ok", cpu.id);
}
// TODO: FIXME: irq overflow
void timer_handler()
{
    arch_timer_mute_irq();

    spin_lock(&timer_handler_lock);

    uint32_t num_of_period = 100;
    // printf("[C%d] 0x%x\n", cpu.id, arch_get_count());
    arch_timer_update(num_of_period);

    spin_unlock(&timer_handler_lock);

    arch_timer_recover_irq();
}