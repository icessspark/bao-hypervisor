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

#include <bao.h>

#include <console.h>
#include <cpu.h>
#include <drivers/virtio_prelude.h>
#include <interrupts.h>
#include <mem.h>
#include <printk.h>
#include <vmm.h>

// FIXME: NET: Registered protocol family 1
// TODO: uboot

void init(uint64_t cpu_id, uint64_t load_addr, uint64_t config_addr)
{
    /**
     * These initializations must be executed first and in fixed order.
     */
    /* NOTE: this is hardcoded for qemu virt machine.
     * We cannot control the $x0 provided by qemu,
     * so far this ugly workaround works.
     * Specify the config_addr in qemu argument like
     * `-device loader,file=configs/virtlinux/virtlinux.bin,addr=0x49000000`
     * */

    config_addr = 0x49000000;
    cpu_init(cpu_id, load_addr);
    mem_init(load_addr, config_addr);

    /* -------------------------------------------------------------- */

    if (cpu.id == CPU_MASTER) {
        console_init();
        INFO("Bao Hypervisor");
    }
    interrupts_init();

    if (cpu.id == CPU_MASTER) {
        virtio_blk_init();
        INFO("virtio_blk_init ok");
    }
    vmm_init();
    /* Should never reach here */
    while (1)
        ;
}
