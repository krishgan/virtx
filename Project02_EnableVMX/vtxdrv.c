/*
 * Copyright Krishna Ganugapati. All Rights Reserved.
 *
 * vtxdrv: Minimal Kernel Driver to Enable VMX
 * This module enables VMX operation on a compatible Intel CPU.
 * It checks CPU capabilities, enables VMX in CR4, and executes VMXON.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/desc.h>
#include <asm/io.h>
#include <asm/virtext.h>
#include <asm/segment.h>

/*
 * This driver performs the following tasks:
 *
 * 1. Checks if the CPU supports VMX using the CPUID instruction.
 * 2. Verifies that VMX is enabled in the IA32_FEATURE_CONTROL MSR (i.e., BIOS allows VMX).
 * 3. Enables VMX operation by setting the VMXE bit in CR4.
 * 4. Allocates a 4KB-aligned VMXON region and writes the VMCS revision ID to it.
 * 5. Executes the VMXON instruction to enter VMX root operation.
 * 6. On module exit, executes VMXOFF to leave VMX mode and frees the allocated memory.
 */

#define VMXON_REGION_SIZE 4096
static void *vmxon_region = NULL;

static inline int cpu_has_vtx_support(void) {
    u32 eax, ebx, ecx, edx;
    cpuid(1, &eax, &ebx, &ecx, &edx);
    return ecx & (1 << 5);  // ECX[5] = VMX
}

static inline u64 read_msr_safe(u32 msr) {
    u64 val;
    rdmsrl(msr, val);
    return val;
}

static int __init vtxdrv_init(void) {
    pr_info("[vtxdrv] Loading minimal VMX driver\n");

    if (!cpu_has_vtx_support()) {
        pr_err("[vtxdrv] CPU does not support VMX\n");
        return -EOPNOTSUPP;
    }

    u64 feature_control = read_msr_safe(MSR_IA32_FEATURE_CONTROL);
    if (!(feature_control & 0x4)) {
        pr_err("[vtxdrv] VMX not enabled in BIOS (IA32_FEATURE_CONTROL MSR)\n");
        return -EACCES;
    }

    cr4_set_bits(X86_CR4_VMXE); // Set VMXE bit in CR4

    vmxon_region = (void *)__get_free_page(GFP_KERNEL);
    if (!vmxon_region) {
        pr_err("[vtxdrv] Failed to allocate VMXON region\n");
        return -ENOMEM;
    }
    memset(vmxon_region, 0, VMXON_REGION_SIZE);

    u64 vmx_msr = read_msr_safe(MSR_IA32_VMX_BASIC);
    *(u32 *)vmxon_region = (u32)vmx_msr; // Write VMCS revision ID

    u64 phys = virt_to_phys(vmxon_region);
    u8 status;
    asm volatile ("vmxon %[pa]; setna %[status]"
                  : [status] "=rm" (status)
                  : [pa] "m" (phys)
                  : "memory", "cc");

    if (status) {
        pr_err("[vtxdrv] VMXON failed\n");
        free_page((unsigned long)vmxon_region);
        return -EIO;
    }

    pr_info("[vtxdrv] VMXON successful! CPU now in VMX operation\n");
    return 0;
}

static void __exit vtxdrv_exit(void) {
    if (vmxon_region) {
        asm volatile ("vmxoff" ::: "memory");
        free_page((unsigned long)vmxon_region);
        pr_info("[vtxdrv] VMXOFF executed. Exiting VMX operation\n");
    }
}

module_init(vtxdrv_init);
module_exit(vtxdrv_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("VirtX Project");
MODULE_DESCRIPTION("vtxdrv: Minimal driver to enable VMX operation on Intel CPUs");
