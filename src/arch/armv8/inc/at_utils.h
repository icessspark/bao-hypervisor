//
// Created by tonny on 8/7/20.
//

#ifndef BAO_AT_UTILS_H
#define BAO_AT_UTILS_H

#include <arch/sysregs.h>
#include <bao.h>
#include <cpu.h>
#include <mem.h>

static inline uint64_t el2_va2pa(uint64_t el2_va)
{
    register uint64_t par, par_saved;
    MRS(par_saved, PAR_EL1);
    asm volatile("AT S1E2R, %0" ::"r"(el2_va));
    MRS(par, PAR_EL1);
    MSR(PAR_EL1, par_saved);
    if (par & PAR_F) {
        // address translate failed
        return 0;
    } else {
        return (par & PAR_PA_MSK) | (el2_va & (PAGE_SIZE - 1));
    }
}

static inline uint64_t ipa2pa(uint64_t ipa)
{
    register uint64_t par, par_saved, sctlr_el1;
    MRS(par_saved, PAR_EL1);
    MRS(sctlr_el1, SCTLR_EL1);
    sctlr_el1 &= ~0b1ull;
    MSR(SCTLR_EL1, sctlr_el1);
    asm volatile("AT S12E1R, %0" ::"r"(ipa));
    MRS(par, PAR_EL1);
    sctlr_el1 |= 0b1ull;
    MSR(SCTLR_EL1, sctlr_el1);
    MSR(PAR_EL1, par_saved);
    if (par & PAR_F) {
        // address translate failed
        return 0;
    } else {
        return (par & PAR_PA_MSK) | (ipa & (PAGE_SIZE - 1));
    }
}

// FIXME: memory leak?
static inline void* ipa2va(uint64_t ipa)
{
    uint64_t pa = ipa2pa(ipa);
    uint64_t offset = pa & (PAGE_SIZE - 1);
    void* va = mem_alloc_vpage(&cpu.as, SEC_HYP_PRIVATE, NULL, 1);
    ppages_t ppages = mem_ppages_get(pa & ~(PAGE_SIZE - 1), 1);
    mem_map(&cpu.as, va, &ppages, 1, PTE_HYP_FLAGS);

    return va + offset;
}

static inline void writeb(uint8_t value, void* addr)
{
    *((uint8_t volatile*)addr) = value;
}

static inline void writew(uint16_t value, void* addr)
{
    *((uint16_t volatile*)addr) = value;
}

static inline void writel(uint32_t value, void* addr)
{
    *((uint32_t volatile*)addr) = value;
}

static inline uint8_t readb(void* addr)
{
    return *((uint8_t volatile*)addr);
}

static inline uint16_t readw(void* addr)
{
    return *((uint16_t volatile*)addr);
}

static inline uint32_t readl(void* addr)
{
    return *((uint32_t volatile*)addr);
}

#endif  // BAO_AT_UTILS_H
