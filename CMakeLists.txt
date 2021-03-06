# This is a dummy CMakeLists for CLion to load

cmake_minimum_required(VERSION 3.9)
project(bao)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON )
add_definitions(-DCMAKE_EXPORT_COMPILE_COMMANDS=ON)

include_directories(
        src/arch/armv8/inc
        src/core/inc
        src/lib/inc
        src/platform/hikey960/inc
        src/platform/ultra96/inc
        src/platform/zcu102/inc
        src/platform/zcu104/inc
)

include_directories(
        src/arch/armv8/inc
        src/core/inc
        src/lib/inc
        src/platform/drivers/pl011_uart/inc
        src/platform/drivers/zynq_uart/inc
        src/platform/drivers/virtio_blk/inc
        src/platform/hikey960/inc
        src/platform/ultra96/inc
        src/platform/zcu102/inc
        src/platform/zcu104/inc
)

add_executable(bao
        configs/example/config.c
        configs/virtlinux/config.c
        src/arch/armv8/aborts.c
        src/arch/armv8/asm_defs.c
        src/arch/armv8/cache.c
        src/arch/armv8/cpu.c
        src/arch/armv8/gicv2.c
        src/arch/armv8/inc/arch/aborts.h
        src/arch/armv8/inc/arch/bao.h
        src/arch/armv8/inc/arch/cache.h
        src/arch/armv8/inc/arch/cpu.h
        src/arch/armv8/inc/arch/fences.h
        src/arch/armv8/inc/arch/gic.h
        src/arch/armv8/inc/arch/interrupts.h
        src/arch/armv8/inc/arch/mem.h
        src/arch/armv8/inc/arch/page_table.h
        src/arch/armv8/inc/arch/platform.h
        src/arch/armv8/inc/arch/psci.h
        src/arch/armv8/inc/arch/smc.h
        src/arch/armv8/inc/arch/spinlock.h
        src/arch/armv8/inc/arch/sysregs.h
        src/arch/armv8/inc/arch/tlb.h
        src/core/inc/virtio_mmio.h
        src/core/inc/virtq.h
        src/core/inc/virt_dev.h
        src/arch/armv8/inc/arch/vgicv2.h
        src/arch/armv8/inc/arch/vm.h
        src/arch/armv8/interrupts.c
        src/arch/armv8/mem.c
        src/arch/armv8/page_table.c
        src/arch/armv8/platform.c
        src/arch/armv8/psci.c
        src/arch/armv8/smc.c
        src/arch/armv8/vgicv2.c
        src/arch/armv8/vm.c
        src/arch/armv8/vmm.c
        src/core/cache.c
        src/core/config.c
        src/core/console.c
        src/core/cpu.c
        src/core/inc/bao.h
        src/core/inc/cache.h
        src/core/inc/config.h
        src/core/inc/console.h
        src/core/inc/cpu.h
        src/core/inc/emul.h
        src/core/inc/fences.h
        src/core/inc/interrupts.h
        src/core/inc/mem.h
        src/core/inc/objcache.h
        src/core/inc/page_table.h
        src/core/inc/platform.h
        src/core/inc/spinlock.h
        src/core/inc/tlb.h
        src/core/inc/vm.h
        src/core/inc/vmm.h
        src/core/init.c
        src/core/interrupts.c
        src/core/mem.c
        src/core/iommu.c
        src/core/objcache.c
        src/core/virtio_mmio.c
        src/core/virt_dev.c
        src/core/virtq.c
        src/core/vm.c
        src/core/vmm.c
        src/lib/bit.c
        src/lib/bitmap.c
        src/lib/inc/bit.h
        src/lib/inc/bitmap.h
        src/lib/inc/list.h
        src/lib/inc/printk.h
        src/lib/inc/string.h
        src/lib/inc/util.h
        src/lib/printk.c
        src/lib/string.c
        src/platform/drivers/pl011_uart/inc/drivers/pl011_uart.h
        src/platform/drivers/pl011_uart/pl011_uart.c
        src/platform/drivers/zynq_uart/inc/drivers/zynq_uart.h
        src/platform/drivers/zynq_uart/zynq_uart.c
        src/platform/hikey960/hikey960_desc.c
        src/platform/hikey960/inc/plat/platform.h
        src/platform/hikey960/inc/plat/psci.h
        src/platform/ultra96/inc/plat/platform.h
        src/platform/ultra96/inc/plat/psci.h
        src/platform/ultra96/ultra96_desc.c
        src/platform/zcu102/inc/plat/platform.h
        src/platform/zcu102/inc/plat/psci.h
        src/platform/zcu102/zcu102_desc.c
        src/platform/zcu104/inc/plat/platform.h
        src/platform/zcu104/inc/plat/psci.h
        src/platform/zcu104/zcu104_desc.c
        src/lib/printf.c
        src/lib/inc/printf.h
        src/platform/drivers/virtio_blk/virtio_blk.c
        src/platform/drivers/virtio_blk/inc/drivers/virtio_blk.h
        src/platform/drivers/virtio_blk/inc/drivers/virtio_mmio.h
        src/platform/drivers/virtio_blk/inc/drivers/virtio_ring.h
        src/platform/drivers/virtio_blk/inc/drivers/virtio_types.h
        src/platform/drivers/virtio_blk/inc/drivers/types.h
        src/platform/drivers/virtio_blk/inc/drivers/virtio.h
        src/arch/armv8/inc/at_utils.h
        src/platform/drivers/virtio_blk/inc/drivers/virtio_prelude.h)