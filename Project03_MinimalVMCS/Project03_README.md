# Project 03 – Minimal VMCS Setup

## Recap of Project 2: Launching the Hypervisor

In Project 2, we introduced our custom kernel module `vtxdrv`, which initialized Intel VT-x (VMX) support from within a Linux kernel module. We implemented a basic character device and added `vtxctl`, a simple user-mode tool that could send configuration data to the kernel module via `ioctl`.

We also allocated a VMXON region, verified VMX availability using CPUID, enabled the `CR4.VMXE` bit, and entered VMX operation via `VMXON`. This was our first step toward constructing a minimal hypervisor framework.

## Project 3 Goals: Creating a VMCS and Defining IOCTLs

In Project 3, we build on our previous work by:

1. Defining a basic VM abstraction using the `vtx_vm_config` structure.
2. Supporting the creation of a VM with configurable guest memory via `ioctl`.
3. Allocating and initializing the **Virtual Machine Control Structure (VMCS)**.
4. Preparing for multiple vCPUs by organizing VMCS instances per vCPU.
5. Laying the groundwork for running guest code and managing VM exits.

## Kernel Driver Changes

- We added `vtx_vm_config` as a configuration structure passed from userspace using `ioctl`. It includes fields like the VM name, size of guest memory, and number of vCPUs.
- We extended the character device handler to support a new `VTXDRV_CREATE_VM` ioctl.
- We allocate and map guest memory in the kernel and prepare the VMCS region.

### Sample `vtx_vm_config` Structure

```c
struct vtx_vm_config {
    char name[32];
    unsigned long guest_mem_size;
    int num_vcpus;
};
```

We validate this structure in `vtxdrv_ioctl` and allocate memory based on its fields.

## IOCTL Design

The following ioctl has been implemented in this project:

```c
#define VTXDRV_CREATE_VM _IOW('k', 1, struct vtx_vm_config)
```

This enables the user-mode `vtxctl` tool to send a VM creation request to the kernel. The kernel then:

- Allocates guest memory
- Allocates a VMCS region
- Initializes it with `VMCS revision ID`
- Tracks per-core VMCS for future expansion

The IOCTL implementation includes logging and safety checks to ensure alignment and correctness.

## VMCS Overview

The **VMCS (Virtual Machine Control Structure)** is a hardware-defined data structure required for VMX operation. It stores control fields and guest/host state for a single logical processor (vCPU).

Each vCPU has a separate VMCS. In this project:

- We allocate a single VMCS region using `__get_free_page`.
- We write the **VMCS Revision ID** (read from `IA32_VMX_BASIC` MSR) to the first 4 bytes.
- We will call `VMPTRLD` in the next project to load the VMCS.

### Fields in the VMCS (for future work)

There are six main classes of fields in a VMCS:

1. **Guest-State Area** – e.g., RIP, RSP, segment registers
2. **Host-State Area** – e.g., CR3, CR4, IDT base
3. **Control Fields** – pin-based, proc-based, VM-exit/entry controls
4. **VM-Exit Information Fields** – VM-exit reason, instruction length, etc.
5. **VM-Entry Fields** – entry controls and guest state on entry
6. **Execution Control Fields** – controls for VMX behavior during guest execution

### VMCS Initialization in Project 3

We are not yet populating these fields in this project. We are just:

- Allocating memory for the VMCS
- Writing the VMX revision ID
- Setting ourselves up for `VMPTRLD` in Project 4

## Multi-vCPU Architecture (Planned)

Each vCPU requires:

- A dedicated VMCS region
- A thread (from userspace) to run that vCPU
- A structure to track guest memory mapping

In a multi-core system, we would issue one `ioctl` per vCPU, each associating with the VM via a shared file descriptor and specifying a vCPU ID.

In Project 3, we support only a single vCPU but establish the abstraction to extend to multi-core VMs.

## What’s Next

In Project 4:

- We will implement `VMPTRLD` and `VMCLEAR`
- Begin setting up guest registers
- Inject a test real-mode or flat-mode guest
- Handle the first `VMEXIT`

We are close to bootstrapping a functional virtual machine.

Stay tuned.
