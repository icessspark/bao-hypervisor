#ifndef __ARCH_TIMER_H__
#define __ARCH_TIMER_H__

#define CTL_ENABLE 1 << 0
#define CTL_IMASK 1 << 1
#define CTL_ISTATUS 1 << 2

#define EL1_PHY_TIMER_INT 30
#define EL1_VIRT_TIMER_INT 27
#define NONSEC_EL2_PHY_TIMER_INT 26
#define NONSEC_EL2_VIRT_TIMER_INT 28
#define EL3_PHY_TIMER_INT 29
#define SEC_EL2_PHY_TIMER_INT 20
#define SEC_EL2_VIRT_TIMER_INT 19

#define TIME_PERIOD 240000

void arch_timer_init(uint64_t cpu_id);
void arch_timer_update(uint32_t n);
void arch_timer_mute_irq();
void arch_timer_recover_irq();
uint64_t arch_get_count();

#endif /* __ARCH_TIEMR */
