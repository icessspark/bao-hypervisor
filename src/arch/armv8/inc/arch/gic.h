/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Angelo Ruocco <angeloruocco90@gmail.com>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#ifndef __GIC_H__
#define __GIC_H__

#include <bao.h>
#include <bitmap.h>
#include <emul.h>
#include <spinlock.h>

#define GIC_MAX_INTERUPTS 1024
#define GIC_MAX_SGIS 16
#define GIC_MAX_PPIS 16
#define GIC_CPU_PRIV (GIC_MAX_SGIS + GIC_MAX_PPIS)
#define GIC_MAX_SPIS (GIC_MAX_INTERUPTS - GIC_CPU_PRIV)
#define GIC_PRIO_BITS 8
#define GIC_TARGET_BITS 8
#define GIC_MAX_TARGETS GIC_TARGET_BITS
#define GIC_CONFIG_BITS 2
#define GIC_SEC_BITS 2
#define GIC_SGI_BITS 8

#define GIC_INT_REG(NINT) (NINT / (sizeof(uint32_t) * 8))
#define GIC_INT_MASK(NINT) (1U << NINT % (sizeof(uint32_t) * 8))
#define GIC_NUM_INT_REGS(NINT) GIC_INT_REG(NINT)
#define GIC_NUM_PRIVINT_REGS (GIC_CPU_PRIV / (sizeof(uint32_t) * 8))

#define GIC_PRIO_REG(NINT) ((NINT * GIC_PRIO_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_PRIO_REGS(NINT) GIC_PRIO_REG(NINT)
#define GIC_PRIO_OFF(NINT) (NINT * GIC_PRIO_BITS) % (sizeof(uint32_t) * 8)

#define GIC_TARGET_REG(NINT) ((NINT * GIC_TARGET_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_TARGET_REGS(NINT) GIC_TARGET_REG(NINT)
#define GIC_TARGET_OFF(NINT) (NINT * GIC_TARGET_BITS) % (sizeof(uint32_t) * 8)

#define GIC_CONFIG_REG(NINT) ((NINT * GIC_CONFIG_BITS) / (sizeof(uint32_t) * 8))
#define GIC_NUM_CONFIG_REGS(NINT) GIC_CONFIG_REG(NINT)
#define GIC_CONFIG_OFF(NINT) (NINT * GIC_CONFIG_BITS) % (sizeof(uint32_t) * 8)

#define GIC_NUM_SEC_REGS(NINT) ((NINT * GIC_SEC_BITS) / (sizeof(uint32_t) * 8))

#define GIC_NUM_SGI_REGS \
    ((GIC_MAX_SGIS * GIC_SGI_BITS) / (sizeof(uint32_t) * 8))
#define GICD_SGI_REG(NINT) (NINT / 4)
#define GICD_SGI_OFF(NINT) ((NINT % 4) * 8)

#define GIC_NUM_APR_REGS ((1UL << (GIC_PRIO_BITS - 1)) / (sizeof(uint32_t) * 8))
#define GIC_NUM_LIST_REGS (64)

/* Distributor Control Register, GICD_CTLR */

#define GICD_CTLR_EN_BIT (0x1)

/*  Interrupt Controller Type Register, GICD_TYPER */

#define GICD_TYPER_ITLINENUM_OFF (0)
#define GICD_TYPER_ITLINENUM_LEN (5)
#define GICD_TYPER_CPUNUM_OFF (5)
#define GICD_TYPER_CPUNUM_LEN (3)
#define GICD_TYPER_SECUREXT_BIT (1UL << 10)
#define GICD_TYPER_LSPI_OFF (11)
#define GICD_TYPER_LSPI_LEN (6)

/* Software Generated Interrupt Register, GICD_SGIR */

#define GICD_TYPER_ITLN_OFF 0
#define GICD_TYPER_ITLN_LEN 5
#define GICD_TYPER_ITLN_MSK BIT_MASK(GICD_TYPER_ITLN_OFF, GICD_TYPER_ITLN_LEN)
#define GICD_TYPER_CPUN_OFF 5
#define GICD_TYPER_CPUN_LEN 3
#define GICD_TYPER_CPUN_MSK BIT_MASK(GICD_TYPER_CPUN_OFF, GICD_TYPER_CPUN_LEN)

#define GICD_SGIR_SGIINTID_OFF 0
#define GICD_SGIR_SGIINTID_LEN 4
#define GICD_SGIR_SGIINTID_MSK \
    (BIT_MASK(GICD_SGIR_SGIINTID_OFF, GICD_SGIR_SGIINTID_LEN))
#define GICD_SGIR_SGIINTID(sgir) \
    bit_extract(sgir, GICD_SGIR_SGIINTID_OFF, GICD_SGIR_SGIINTID_LEN)
#define GICD_SGIR_CPUTRGLST_OFF 16
#define GICD_SGIR_CPUTRGLST_LEN 8
#define GICD_SGIR_CPUTRGLST(sgir) \
    bit_extract(sgir, GICD_SGIR_CPUTRGLST_OFF, GICD_SGIR_CPUTRGLST_LEN)
#define GICD_SGIR_TRGLSTFLT_OFF 24
#define GICD_SGIR_TRGLSTFLT_LEN 2
#define GICD_SGIR_TRGLSTFLT(sgir) \
    bit_extract(sgir, GICD_SGIR_TRGLSTFLT_OFF, GICD_SGIR_TRGLSTFLT_LEN)

typedef struct {
    /* Enables interrupts and affinity routing.*/
    uint32_t CTLR;  
    /* Provides information about what features the GIC implementation supports. It indicates:
    - Whether the GIC implementation supports two Security states.
    - The maximum number of INTIDs that the GIC implementation supports.
    - The number of PEs that can be used as interrupt targets.*/
    uint32_t TYPER;  
    /* Provides information about the implementer and revision of the Distributor.*/
    uint32_t IIDR;  
    uint8_t pad0[0x0080 - 0x000C];
    /* banked CPU */
    uint32_t IGROUPR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    /* Enables forwarding of the corresponding interrupt to the CPU interfaces.*/
    uint32_t ISENABLER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)]; 
    /* Disables forwarding of the corresponding interrupt to the CPU interfaces.*/
    uint32_t ICENABLER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)]; 
    /* Adds the pending state to the corresponding interrupt.*/
    uint32_t ISPENDR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];  
    /* Removes the pending state from the corresponding interrupt.
    Next two registers are used when saving and restoring GIC state.*/
    uint32_t ICPENDR[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    /* Activates the corresponding interrupt. */
    uint32_t ISACTIVER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    /* Deactivates the corresponding interrupt*/
    uint32_t ICACTIVER[GIC_NUM_INT_REGS(GIC_MAX_INTERUPTS)];
    /* Holds the priority of the corresponding interrupt.*/
    uint32_t IPRIORITYR[GIC_NUM_PRIO_REGS(GIC_MAX_INTERUPTS)];  
    // uint8_t pad1[0x0800 - 0x07FC];
    /* holds the list of CPU interfaces to which the Distributor forwards the interrupt.*/
    uint32_t ITARGETSR[GIC_NUM_TARGET_REGS(GIC_MAX_INTERUPTS)];  
    // uint8_t pad2[0x0C00 - 0x0820];
    /* Determines whether the corresponding interrupt is edge-triggered or level-sensitive.*/
    uint32_t ICFGR[GIC_NUM_CONFIG_REGS(GIC_MAX_INTERUPTS)];  
    uint8_t pad3[0x0E00 - 0x0D00];
    /* Enables Secure software to permit Non-secure software on a particular PE to create and control Group 0 interrupts.*/
    uint32_t NSACR[GIC_NUM_SEC_REGS(GIC_MAX_INTERUPTS)]; 
    /* Controls the generation of SGIs.*/
    uint32_t SGIR;  
    uint8_t pad4[0x0F10 - 0x0F04];
    /* Removes the pending state from an SGI.*/
    uint32_t CPENDSGIR[GIC_NUM_SGI_REGS]; 
    /* Adds the pending state to an SGI.  */
    uint32_t SPENDSGIR[GIC_NUM_SGI_REGS];   
} __attribute__((__packed__)) gicd_t;

/* CPU Interface Control Register, GICC_CTLR */

#define GICC_CTLR_EN_BIT (0x1)
#define GICC_CTLR_EOImodeNS_BIT (1UL << 9)
#define GICC_CTLR_WR_MSK (0x1)
#define GICC_IAR_ID_OFF (0)
#define GICC_IAR_ID_LEN (10)
#define GICC_IAR_ID_MSK (BIT_MASK(GICC_IAR_ID_OFF, GICC_IAR_ID_LEN))
#define GICC_IAR_CPU_OFF (10)
#define GICC_IAR_CPU_LEN (3)
#define GICC_IAR_CPU_MSK (BIT_MASK(GICC_IAR_CPU_OFF, GICC_IAR_CPU_LEN))

typedef struct {
    /*Controls the CPU interface, including enabling of interrupt groups, 
    interrupt signal bypass, binary point registers used, 
    and separation of priority drop and interrupt deactivation.*/
    uint32_t CTLR;
    /* provides an interrupt priority filter. 
    Only interrupts with higher priority than the value in this register are signaled to the PE.*/
    uint32_t PMR;
    /* Defines the point at which the priority value fields split into two parts, 
    the group priority field and the subpriority field.*/
    uint32_t BPR;
    /* Provides the INTID of the signaled interrupt. 
    A read of this register by the PE acts as an acknowledge for the interrupt.*/
    uint32_t IAR;
    /* A write to this register performs priority drop for the specified interrupt and, 
    if the appropriate GICC_CTLR.EOImodeS or GICC_CTLR.EOImodeNS field == 0, also deactivates the interrupt.*/
    uint32_t EOIR;
    /* indicates the running priority of the CPU interface.*/
    uint32_t RPR;
    /* Provides the INTID of the highest priority pending interrupt on the CPU interface.*/
    uint32_t HPPIR;
    /* Defines the point at which the priority value fields split into two parts, 
    the group priority field and the subpriority field. 
    The group priority field determines Group 1 interrupt preemption.*/
    uint32_t ABPR;
    /* Provides the INTID of the signaled Group 1 interrupt. 
      A read of this register by the PE acts as an acknowledge for the interrupt.*/
    uint32_t AIAR;
    /*A write to this register performs priority drop for the specified Group 1 interrupt and, 
    if the appropriate GICC_CTLR.EOImodeS or GICC_CTLR.EOImodeNS field == 0, also deactivates the interrupt.*/
    uint32_t AEOIR;
    /* If the highest priority pending interrupt is in Group 1, 
    this register provides the INTID of the highest priority pending interrupt on the CPU interface.*/
    uint32_t AHPPIR;
    uint8_t pad0[0x00D0 - 0x002C];
    /* Provides information about interrupt active priorities.*/
    uint32_t APR[GIC_NUM_APR_REGS];
    /* Provides information about Group 1 interrupt active priorities.*/
    uint32_t NSAPR[GIC_NUM_APR_REGS];
    uint8_t pad1[0x00FC - 0x00F0];
    /* Provides information about the implementer and revision of the CPU Interface.*/
    uint32_t IIDR;
    uint8_t pad2[0x1000 - 0x0100];
    /*When interrupt priority drop is separated from interrupt deactivation, 
    a write to this register deactivates the specified interrupt.*/
    uint32_t DIR;
} __attribute__((__packed__)) gicc_t;

#define GICH_HCR_En_BIT (1 << 0)
#define GICH_HCR_UIE_BIT (1 << 1)
#define GICH_HCR_LRENPIE_BIT (1 << 2)
#define GICH_HCR_NPIE_BIT (1 << 3)
#define GICH_HCR_VGrp0DIE_BIT (1 << 4)
#define GICH_HCR_VGrp0EIE_BIT (1 << 5)
#define GICH_HCR_VGrp1EIE_BIT (1 << 6)
#define GICH_HCR_VGrp1DIE_BIT (1 << 7)
#define GICH_HCR_EOICount_OFF (27)
#define GICH_HCR_EOICount_LEN (5)
#define GICH_HCR_EOICount_MASK \
    BIT_MASK(GICH_HCR_EOICount_OFF, GICH_HCR_EOICount_LEN)

#define GICH_VTR_OFF (0)
#define GICH_VTR_LEN (6)
#define GICH_VTR_MSK BIT_MASK(GICH_VTR_OFF, GICH_VTR_LEN)

#define GICH_LR_VID_OFF (0)
#define GICH_LR_VID_LEN (10)
#define GICH_LR_VID_MSK BIT_MASK(GICH_LR_VID_OFF, GICH_LR_VID_LEN)
#define GICH_LR_VID(LR) (bit_extract(LR, GICH_LR_VID_OFF, GICH_LR_VID_LEN))

#define GICH_LR_PID_OFF (10)
#define GICH_LR_PID_LEN (10)
#define GICH_LR_PID_MSK BIT_MASK(GICH_LR_PID_OFF, GICH_LR_PID_LEN)
#define GICH_LR_CPUID_OFF (10)
#define GICH_LR_CPUID_LEN (3)
#define GICH_LR_CPUID_MSK BIT_MASK(GICH_LR_CPUID_OFF, GICH_LR_CPUID_LEN)
#define GICH_LR_CPUID(LR) \
    (bit_extract(LR, GICH_LR_CPUID_OFF, GICH_LR_CPUID_LEN))
#define GICH_LR_PRIO_OFF (23)
#define GICH_LR_PRIO_LEN (5)
#define GICH_LR_PRIO_MSK BIT_MASK(GICH_LR_PRIO_OFF, GICH_LR_PRIO_LEN)
#define GICH_LR_STATE_OFF (28)
#define GICH_LR_STATE_LEN (2)
#define GICH_LR_STATE_MSK BIT_MASK(GICH_LR_STATE_OFF, GICH_LR_STATE_LEN)
#define GICH_LR_STATE(LR) \
    (bit_extract(LR, GICH_LR_STATE_OFF, GICH_LR_STATE_LEN))

#define GICH_LR_STATE_INV ((0 << GICH_LR_STATE_OFF) & GICH_LR_STATE_MSK)
#define GICH_LR_STATE_PND ((1 << GICH_LR_STATE_OFF) & GICH_LR_STATE_MSK)
#define GICH_LR_STATE_ACT ((2 << GICH_LR_STATE_OFF) & GICH_LR_STATE_MSK)
#define GICH_LR_STATE_ACTPEND ((3 << GICH_LR_STATE_OFF) & GICH_LR_STATE_MSK)

#define GICH_LR_HW_BIT (1U << 31)
#define GICH_LR_EOI_BIT (1U << 19)

#define GICH_MISR_EOI (1U << 0)
#define GICH_MISR_U (1U << 1)
#define GICH_MISR_LRPEN (1U << 2)
#define GICH_MISR_NP (1U << 3)
#define GICH_MISR_VGrp0E (1U << 4)
#define GICH_MISR_VGrp0D (1U << 5)
#define GICH_MISR_VGrp1E (1U << 6)
#define GICH_MISR_VGrp1D (1U << 7)

typedef struct {
    /* Controls the virtual CPU interface.*/
    uint32_t HCR;
    /* Indicates the number of implemented virtual priority bits and List registers.*/
    uint32_t VTR;
    /*Enables the hypervisor to save and restore the virtual machine view of the GIC state. 
    This register is updated when a virtual machine updates the virtual CPU interface registers.*/
    uint32_t VMCR;
    uint8_t pad0[0x10 - 0x0c];
    /* Indicates which maintenance interrupts are asserted.*/
    uint32_t MISR;
    uint8_t pad1[0x20 - 0x14];
    /* Indicates which List registers have outstanding EOI maintenance interrupts.*/
    uint32_t EISR[GIC_NUM_LIST_REGS / (sizeof(uint32_t) * 8)];
    uint8_t pad2[0x30 - 0x28];
    /* Indicates which List registers contain valid interrupts.*/
    uint32_t ELSR[GIC_NUM_LIST_REGS / (sizeof(uint32_t) * 8)];
    uint8_t pad3[0xf0 - 0x38];
    /*These registers track which preemption levels are active in the virtual CPU interface, 
    and indicate the current active priority. 
    Corresponding bits are set to 1 in this register when an interrupt is acknowledged, 
    based on GICH_LR<n>.Priority, and the least significant bit set is cleared on EOI.*/
    uint32_t APR;
    uint8_t pad4[0x100 - 0x0f4];
    /* These registers provide context information for the virtual CPU interface.*/
    uint32_t LR[GIC_NUM_LIST_REGS];
} __attribute__((__packed__)) gich_t;

typedef struct {
    uint32_t CTLR;
    uint32_t PMR;
    uint32_t BPR;
    uint32_t IAR;
    uint32_t EOIR;
    uint32_t RPR;
    uint32_t HPPIR;
    uint32_t ABPR;
    uint32_t AIAR;
    uint32_t AEOIR;
    uint32_t AHPPIR;
    uint8_t pad0[0xD0 - 0x2C];
    uint32_t APR[GIC_NUM_APR_REGS];
    uint8_t pad1[0x00FC - 0x00E0];
    uint32_t IIDR;
    uint8_t pad2[0x1000 - 0x0100];
    uint32_t DIR;
} __attribute__((__packed__)) gicv_t;


extern volatile gicd_t gicd;
extern volatile gicc_t gicc;
extern volatile gich_t gich;

enum int_state { INV, PEND, ACT, PENDACT };

typedef struct {
    uint32_t CTLR;
    uint32_t PMR;
    uint32_t BPR;
    uint32_t IAR;
    uint32_t EOIR;
    uint32_t RPR;
    uint32_t HPPIR;
    uint32_t priv_ISENABLER;
    uint32_t priv_IPRIORITYR[GIC_NUM_PRIO_REGS(GIC_CPU_PRIV)];

    uint32_t HCR;
    uint32_t LR[GIC_NUM_LIST_REGS];
} gicc_state_t;

extern uint64_t NUM_LRS;

void gic_init();
void gic_cpu_init();
void gicd_send_sgi(uint64_t cpu_target, uint64_t sgi_num);

void gicc_save_state(gicc_state_t *state);
void gicc_restore_state(gicc_state_t *state);

void gicd_set_enable(uint64_t int_id, bool en);
void gicd_set_prio(uint64_t int_id, uint8_t prio);
void gicd_set_icfgr(uint64_t int_id, uint8_t cfg);
void gicd_set_act(uint64_t int_id, bool act);
void gicd_set_state(uint64_t int_id, enum int_state state);
void gicd_set_trgt(uint64_t int_id, uint8_t trgt);
uint64_t gicd_get_prio(uint64_t int_id);
enum int_state gicd_get_state(uint64_t int_id);

/*The ITLinesNumber field only indicates the maximum number of SPIs that the GIC implementation might support.
The GIC architecture does not require a GIC implementation to support a continuous range of SPI interrupt IDs. 
Software must check which SPI INTIDs are supported, up to the maximum value indicated by GICD_TYPER.ITLinesNumber.*/
static inline uint64_t gic_num_irqs()
{
    uint32_t itlinenumber =
        bit_extract(gicd.TYPER, GICD_TYPER_ITLN_OFF, GICD_TYPER_ITLN_LEN);
    return 32 * itlinenumber + 1;
}

static inline bool gic_is_sgi(uint64_t int_id)
{
    return int_id < GIC_MAX_SGIS;
}

static inline bool gic_is_priv(uint64_t int_id)
{
    return int_id < GIC_CPU_PRIV;
}

// The number of implemented List registers, minus one.
static inline uint64_t gich_num_lrs()
{
    return ((gich.VTR & GICH_VTR_MSK) >> GICH_VTR_OFF) + 1;
}

#endif /* __GIC_H__ */
