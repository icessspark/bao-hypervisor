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

#ifndef __BAO_H__
#define __BAO_H__

#include <arch/bao.h>

#ifndef __ASSEMBLER__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <printf.h>
#include <util.h>

#define INFO(args...)            \
    {                            \
        printf("[INFO] " args);   \
        printf("\n");            \
    }

#define WARNING(args...)          \
    {                             \
        printf("[WARNING] " args); \
        printf("\n");             \
    }

#define ERROR(args...)          \
    {                           \
        printf("[ERROR] " args); \
        printf("\n");           \
        while (1)               \
            ;                   \
    }


#endif /* __ASSEMBLER__ */

#endif /* __BAO_H__ */
