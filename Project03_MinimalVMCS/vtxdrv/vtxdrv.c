// SPDX-License-Identifier: GPL-2.0
// Copyright Krishna Ganugapati. All Rights Reserved.

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/msr.h>
#include <asm/processor.h>
#include <asm/special_insns.h>
#include <asm/vmx.h>
#include "../include/vtxdrv.h"

static int major;
static void *vmxon_region;
static void *vmcs_region;
static struct vtx_vm_config g_vm_config;

static int vtxdrv_open(struct inode *inode, struct file *file) {
    return 0;
}

static int vtxdrv_release(struct inode *inode, struct file *file) {
    return 0;
}

static long vtxdrv_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case VTX_IOCTL_CREATE_VM:
            if (copy_from_user(&g_vm_config, (void __user *)arg, sizeof(g_vm_config)))
                return -EFAULT;

            pr_info("Creating VM: %s with mem %lu\n", g_vm_config.name, g_vm_config.guest_mem_size);

            u64 basic = __rdmsr(MSR_IA32_VMX_BASIC);
            u32 revision_id = basic & 0x7fffffff;

            vmcs_region = (void *)__get_free_page(GFP_KERNEL);
            if (!vmcs_region)
                return -ENOMEM;

            *(u32 *)vmcs_region = revision_id;
            break;
    }
    return 0;
}

static struct file_operations vtxdrv_fops = {
    .owner = THIS_MODULE,
    .open = vtxdrv_open,
    .release = vtxdrv_release,
    .unlocked_ioctl = vtxdrv_ioctl,
};

static int __init vtxdrv_init(void) {
    major = register_chrdev(0, DEVICE_NAME, &vtxdrv_fops);
    if (major < 0)
        return major;

    u64 feature_control = __rdmsr(MSR_IA32_FEATURE_CONTROL);
    if (!(feature_control & 0x4)) {
        pr_err("VMX not enabled in BIOS\n");
        unregister_chrdev(major, DEVICE_NAME);
        return -EOPNOTSUPP;
    }

    write_cr4(read_cr4() | X86_CR4_VMXE);

    u64 vmx_basic = __rdmsr(MSR_IA32_VMX_BASIC);
    u32 revision_id = vmx_basic & 0x7fffffff;

    vmxon_region = (void *)__get_free_page(GFP_KERNEL);
    if (!vmxon_region)
        return -ENOMEM;

    *(u32 *)vmxon_region = revision_id;
    phys_addr_t vmxon_phys = virt_to_phys(vmxon_region);

    if (__vmxon(&vmxon_phys)) {
        pr_err("VMXON failed\n");
        free_page((unsigned long)vmxon_region);
        return -EIO;
    }

    pr_info("vtxdrv loaded with major %d\n", major);
    return 0;
}

static void __exit vtxdrv_exit(void) {
    if (vmxon_region) {
        __vmxoff();
        free_page((unsigned long)vmxon_region);
    }

    if (vmcs_region)
        free_page((unsigned long)vmcs_region);

    unregister_chrdev(major, DEVICE_NAME);
    pr_info("vtxdrv unloaded\n");
}

module_init(vtxdrv_init);
module_exit(vtxdrv_exit);
MODULE_LICENSE("GPL");