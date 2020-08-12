/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __ARCH_CPU_H__
#define __ARCH_CPU_H__

#include <arch/psci.h>
#include <bao.h>

#define CPU_MASTER (0UL)
#define CPU_MAX (8UL)

typedef struct cpu_arch {
    psci_off_state_t psci_off_state;
} cpu_arch_t;

uint64_t cpu_id_to_mpdir(uint64_t id);

#endif /* __ARCH_CPU_H__ */
