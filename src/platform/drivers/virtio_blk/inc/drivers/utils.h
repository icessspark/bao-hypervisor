//
// Created by tonny on 8/7/20.
//

#ifndef BAO_UTILS_H
#define BAO_UTILS_H

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
    return el2_va2pa(ipa);
}

static inline void writeb(u8 value, void* addr) {
    *((u8 volatile *)addr) = value;
}

static inline void writew(u16 value, void* addr) {
    *((u16 volatile *)addr) = value;
}

static inline void writel(u32 value, void* addr) {
    *((u32 volatile *)addr) = value;
}

static inline u8 readb(void* addr) {
    return *((u8 volatile *)addr);
}

static inline u16 readw(void* addr) {
    return *((u16 volatile *)addr);
}

static inline u32 readl(void* addr) {
    return *((u32 volatile *)addr);
}

#endif  // BAO_UTILS_H
