#ifndef __ARCH_TIMER_H__
#define __ARCH_TIMER_H__

#include <arch/sysregs.h>
#include <fences.h>

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

#define TIME_PERIOD 625000 /*10ms*/

static inline void arch_timer_init(uint64_t cpu_id)
{
    MSR(CNTHP_CTL_EL2, 0x3 & 1 | ~CTL_IMASK);
    /*if the init time period is too small, could get trap when still in el2*/
    MSR(CNTHP_TVAL_EL2, TIME_PERIOD * 500);
}

static inline void arch_timer_update(uint32_t n)
{
    MSR(CNTHP_TVAL_EL2, TIME_PERIOD * n);
    ISB();
}

static inline void arch_timer_mute_irq()
{
    MSR(CNTHP_CTL_EL2, 2);
    ISB();
}

static inline void arch_timer_recover_irq()
{
    MSR(CNTHP_CTL_EL2, 1);
    ISB();
}

static inline uint64_t arch_get_count()
{
    uint64_t cnt = 0;
    MRS(cnt, CNTPCT_EL0);
    ISB();

    return cnt;
}

static inline uint32_t arch_get_frq()
{
    uint32_t feq = 0;

    MRS(feq, CNTFRQ_EL0);
    ISB();

    return feq;
}

#endif /* __ARCH_TIEMR */
