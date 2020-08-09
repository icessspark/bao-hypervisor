//
// Created by tonny on 8/7/20.
//

#ifndef BAO_AT_UTILS_H
#define BAO_AT_UTILS_H

#include <bao.h>
#include <arch/sysregs.h>

static inline uint64_t el2_va2pa(uint64_t el2_va) {
    register uint64_t par, par_saved;
    MRS(par_saved, PAR_EL1);
    asm volatile ("AT S1E2R, %0" :: "r"(el2_va));
    MRS(par, PAR_EL1);
    MSR(PAR_EL1, par_saved);
    if (par & PAR_F) {
        // address translate failed
        return 0;
    } else {
        return (par & PAR_PA_MSK) | (el2_va & (PAGE_SIZE - 1));
    }
}

static inline uint64_t ipa2pa(uint64_t ipa) {
    register uint64_t par, par_saved;
    MRS(par_saved, PAR_EL1);
    asm volatile ("AT S12E1R, %0" :: "r"(ipa));
    MRS(par, PAR_EL1);
    MSR(PAR_EL1, par_saved);
    if (par & PAR_F) {
        // address translate failed
        return 0;
    } else {
        return (par & PAR_PA_MSK) | (ipa & (PAGE_SIZE - 1));
    }
}

static inline void writeb(uint8_t value, void* addr) {
    *((uint8_t volatile *)addr) = value;
}

static inline void writew(uint16_t value, void* addr) {
    *((uint16_t volatile *)addr) = value;
}

static inline void writel(uint32_t value, void* addr) {
    *((uint32_t volatile *)addr) = value;
}

static inline uint8_t readb(void* addr) {
    return *((uint8_t volatile *)addr);
}

static inline uint16_t readw(void* addr) {
    return *((uint16_t volatile *)addr);
}

static inline uint32_t readl(void* addr) {
    return *((uint32_t volatile *)addr);
}

#endif  // BAO_AT_UTILS_H
