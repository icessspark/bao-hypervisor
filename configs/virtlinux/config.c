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

#include <config.h>

/**
 * Declare VM images using the VM_IMAGE macro, passing an identifier and the
 * path for the image.
 */
VM_IMAGE(vm1,../lloader/linux.bin);

struct config config =
    {

        CONFIG_HEADER

            .vmlist_size = 1,
        .vmlist =
            {
                {
                    .image = {.base_addr = 0x40000000,
                              .load_addr = VM_IMAGE_OFFSET(vm1),
                              .size = VM_IMAGE_SIZE(vm1)},

                    .entry = 0x40000000,
                    .cpu_affinity = 0b1,
                    .platform = {.cpu_num = 1,

                                 .region_num = 1,
                                 .regions =
                                     (struct mem_region[]){{.base = 0x40000000,
                                                            .size = 0x8000000}},

                                 .dev_num = 1,
                                 .devs =
                                     (struct dev_region[]){
                                         {.pa = 0x09100000,
                                          .va = 0x09000000,
                                          .size = 0x1000,
                                          .interrupt_num = 2,
                                          .interrupts =
                                              (uint64_t[]){27, 42}}},
                 /* Note:
                  * IRQ 27 is PPI [16,32) interrupt ~ Timer
                  * IRQ 42 is SPI 10 (32 + 10) interrupt ~ UART1
                  * UART1 is specified in a special multi-uart qemu:
                  * https://github.com/tonnylyz/qemu/tree/multi-uart
                  * */

                                 .arch = {.gic = {.gicc_addr = 0x08010000,
                                                  .gicd_addr = 0x08000000}}},
                },
            },
};