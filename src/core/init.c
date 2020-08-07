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

#include <cpu.h>
#include <mem.h>
#include <interrupts.h>
#include <console.h>
#include <printk.h>
#include <vmm.h>
#include <drivers/virtio_prelude.h>


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
        printk("Bao Hypervisor\n\r");
    }
    interrupts_init();

    if (cpu.id == CPU_MASTER) {
        virtio_blk_init();
        printk("virtio_blk_init ok\n");
        char buf[512];
        virtio_blk_read(8, 1, buf);
        printk("virtio_blk_read ok\n");
        int i;
        for (i = 0; i < 512 ; i++) {
            printk("%02x ", buf[i]);
            if ((i + 1) % 32 == 0) {
                printk("\n");
            }
        }
    }
    vmm_init();

    /* Should never reach here */
    while (1);
}
