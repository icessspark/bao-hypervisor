#include <arch/sysregs.h>
#include <arch/timer.h>
#include <fences.h>

void arch_timer_init(uint64_t cpu_id)
{
    MSR(CNTHP_CTL_EL2, 0x3 & 1 | ~CTL_IMASK);
    /*if the init time period is too small, could get trap when still in el2*/
    MSR(CNTHP_TVAL_EL2, TIME_PERIOD * 100);
}

void arch_timer_update(uint32_t n)
{
    MSR(CNTHP_TVAL_EL2, TIME_PERIOD * n);
    ISB();
}

void arch_timer_mute_irq()
{
    MSR(CNTHP_CTL_EL2, 2);
    ISB();
}

void arch_timer_recover_irq()
{
    MSR(CNTHP_CTL_EL2, 1);
    ISB();
}

uint64_t arch_get_count() {
    uint64_t cnt = 0;
    MRS(cnt, CNTPCT_EL0);
    ISB();

    return cnt;
}