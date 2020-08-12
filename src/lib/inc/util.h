/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __UTIL_H__
#define __UTIL_H__

#include <bao.h>
#include <arch/sysregs.h>

/* UTILITY MACROS */

/* align VAL to TO which must be power a two */
#define ALIGN(VAL, TO) ((((VAL) + (TO)-1) / (TO)) * TO)
#define NUM_PAGES(SZ) (ALIGN(SZ, PAGE_SIZE) / PAGE_SIZE)

#define SR_OR(VAL, SHIFT) (((VAL) >> (SHIFT)) | VAL)
/* Next Power Of Two */
#define NPOT(VAL)                                                     \
    ((SR_OR(((VAL)-1), 1) | SR_OR(SR_OR(((VAL)-1), 1), 2) |           \
      SR_OR(SR_OR(SR_OR(((VAL)-1), 1), 2), 4) |                       \
      SR_OR(SR_OR(SR_OR(SR_OR(((VAL)-1), 1), 2), 4), 8) |             \
      SR_OR(SR_OR(SR_OR(SR_OR(SR_OR(((VAL)-1), 1), 2), 4), 8), 16)) + \
     1)

/* Previous Power Of Two */
#define PPOT(VAL) (NPOT((VAL)) - (NPOT((VAL)) >> 1))

#ifndef __ASSEMBLER__

#define max(n1, n2) (((n1) > (n2)) ? (n1) : (n2))
#define min(n1, n2) (((n1) < (n2)) ? (n1) : (n2))

/* WARNING! does not check for overflow! */
#define range_in_range(_base1, _size1, _base2, _size2) \
    (((_base1) >= (_base2)) && ((_base1 + _size1) <= ((_base2) + (_size2))))

/* WARNING! does not check for overflow! */
#define in_range(_addr, _base, _size) range_in_range(_addr, 0, _base, _size)

#endif

static inline uint32_t u64_low_to_u32(uint64_t n64)
{
    return n64 & 0xffffffff;
}

static inline uint32_t u64_high_to_u32(uint64_t n64)
{
    return (n64 & 0xffffffff00000000) >> 32;
}

static inline uint64_t u32_to_u64_low(uint32_t n32)
{
    return (uint64_t)(n32);
}

static inline uint64_t u32_to_u64_high(uint32_t n32)
{
    return (uint64_t)(n32) << 32;
}

#endif /* __UTIL_H__ */