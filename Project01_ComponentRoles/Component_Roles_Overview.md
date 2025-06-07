# Project 1: Software Component Roles and Evolution

## Goal
Establish a clear mental model for the roles of the three components — VTXCTL, VTXDRV, and NanoX — and how they evolve across the VirtX project series.

---

## Overview of the Three Components

### **1. VTXCTL — The User-Space Virtual Machine Launcher and Emulator**
**Primary Role:** VTXCTL acts as a user-mode virtual machine manager. Conceptually, it emulates a primitive hardware system entirely in user space:

- A contiguous region of user-space memory is treated as the **physical RAM** of the guest machine. The guest binary (NanoX or Linux kernel) is loaded into this region.
- Each guest vCPU is represented by a **user-space thread**. These are software representations of CPU cores and are mapped to physical CPU cores by the host OS.
- VTXCTL is responsible for interacting with the kernel-mode hypervisor (VTXDRV) through `ioctl` calls, requesting operations like VM entry and guest interrupt injection.

Our emulated machine is minimal — it has **no hard disk** or PCI devices. It requires only three essential devices to be functional:

1. **Serial Port (UART)** — used for all input/output communication between the guest and host.
2. **Interrupt Controller (PIC)** — manages delivery of hardware interrupts (e.g., timer, keyboard) to the CPU.
3. **Programmable Interval Timer (PIT)** — generates periodic timer interrupts to simulate time passage.

VTXCTL provides software representations of these devices and exposes them via shared memory regions or I/O intercepts to the hypervisor. It acts as a control plane, memory loader, and guest manager.

**Initial Capabilities:**
- Load a guest binary (e.g., NanoX real-mode binary or Linux bzImage) into memory.
- Create a basic vCPU via an `ioctl`.
- Trigger guest entry using `VMLAUNCH` via the hypervisor.

**Evolution Path:**
- Add support for multiple vCPUs using threads.
- Manage vCPU synchronization and scheduling.
- Support advanced guest boot (e.g., Linux kernel + initrd setup).

---

### **2. VTXDRV — The Kernel Hypervisor Driver**
**Primary Role:** VTXDRV is a kernel-mode driver that leverages Intel's VT-x virtualization extensions to execute the user-space emulated machine (from VTXCTL) on actual hardware. It takes the virtual CPU threads and guest memory image created in user space, and maps them into real execution contexts supported by the CPU.

VTXDRV is responsible for:
- Managing the transition into and out of virtualization mode.
- Programming and maintaining VMCS (Virtual Machine Control Structures) for each vCPU.
- Handling all VMEXITs when the guest exits due to I/O, HLT, or exceptions.

To support this, the driver uses three key VT-x instructions:

1. **VMXON**: Transitions the processor into VMX operation mode, enabling access to all other virtualization features. This instruction must be executed before any virtualization activity begins.
2. **VMXOFF**: Exits VMX operation mode, returning the processor to its normal state.
3. **VMLAUNCH/VMRESUME**: Starts or resumes guest execution in VMX non-root mode.

Historically, x86 CPUs were not designed for virtualization. VT-x was introduced as a **hardware bolt-on** to make virtualization efficient and correct. These extensions introduced a **new execution mode split**:

- **Root Mode**: Used by the host and VTXDRV. The processor runs privileged host code and manages virtualization.
- **Non-Root Mode**: Used by the guest OS. The CPU runs guest code until a condition triggers a **VMEXIT**, causing control to return to the host (VTXDRV).

Common VMEXIT triggers include:
- I/O instructions (e.g., `IN`, `OUT`)
- Executing privileged instructions
- Exceptions and interrupts
- Explicit instructions like `HLT`

VTXDRV receives control on VMEXIT, inspects the reason, and either emulates the requested operation or modifies state to resume guest execution. It is the glue layer between the software-emulated machine in user space and the real CPU's virtualization capabilities.

While device emulation (PIC, PIT, UART) is typically handled in the hypervisor driver for accuracy and control, in early stages of development we may implement parts of the emulation in user space (within VTXCTL) for ease of debugging and iteration. As the project matures, responsibility for critical device behavior — such as interrupt generation, timer tick emulation, and serial input/output handling — will migrate to the VTXDRV kernel module, enabling tighter integration with VMEXITs and hardware-based timing. This phased approach allows rapid prototyping while preserving a clean, accurate virtualization path for production.

**Initial Capabilities:**
- Enable and disable VMX operation.
- Allocate VMXON and VMCS regions.
- Set up minimal host and guest state.

**Evolution Path:**
- Configure guest register state and control structures.
- Handle VMEXITs for I/O (`IN`, `OUT`, `HLT`, etc.).
- Emulate legacy devices: PIT, PIC, UART.
- Inject interrupts.
- Emulate Local APIC and support SMP startup.

---

### **3. NanoX — The Guest Operating System**
**Primary Role:** NanoX is a minimal guest OS crafted to demonstrate the x86 boot process and evolve gradually from a 16-bit real-mode program to a 32-bit protected-mode operating system written in C. It starts with the simplest possible execution model and progressively incorporates more sophisticated architectural features of the x86 platform.

At power-on, all x86 (and x86_64) CPUs begin in **real mode**, a 16-bit environment with segmented addressing and no memory protection. NanoX follows this same convention. The earliest version of NanoX will be a simple 16-bit binary written in x86 assembly:

- It will perform direct memory I/O and basic console operations (reading and writing characters).
- It will avoid interrupts or BIOS calls and rely solely on direct `IN` and `OUT` instructions.
- The focus is to illustrate how a CPU can begin executing code immediately after reset.

From this foundation, NanoX will transition to **32-bit protected mode** without virtual memory:
- We will define a Global Descriptor Table (GDT) to create a flat 32-bit address space.
- Local Descriptor Tables (LDTs) will be introduced for task-specific segments if needed.
- The paging system will remain disabled to avoid the complexity of managing page tables, TLBs, and EPTs (Extended Page Tables).

Once in protected mode, NanoX will evolve to support a minimal C runtime environment:
- A `main()` entry point will be established.
- Simple `print` and `scanf`-like functions will be implemented to interact with the console.
- The program will be structured into a basic kernel with a command shell and interrupt handlers.

This evolution offers a clear path from bare-metal 16-bit startup to a modern-feeling 32-bit flat-address OS, teaching core operating system concepts and systems programming in manageable steps.

**Initial Capabilities:**
- Real-mode binary that runs a few `OUT` instructions to a serial port.

**Evolution Path:**
- Add real-mode bootloader that transitions to protected mode.
- Add GDT setup, `main()` entry point, and minimal libc-style print/scan functions.
- Set up IDT and interrupt handlers for IRQ0/IRQ1.
- Build a shell capable of basic input commands.

---

## Interplay and Boundaries

- **VTXCTL** is the orchestrator. It creates and manages virtual CPUs and memory layout but doesn’t touch low-level hardware state directly.
- **VTXDRV** acts as the secure virtualization controller. It translates requests from VTXCTL into hardware-level VT-x state, trapping and emulating as necessary.
- **NanoX** behaves like a real operating system but lives entirely within the illusion created by VTXDRV.

---

## Device Emulation Ownership Table

| Device           | Early Implementation | Final Ownership | Notes                                                                 |
|------------------|----------------------|------------------|-----------------------------------------------------------------------|
| Serial Port (UART) | VTXCTL (user space)  | VTXDRV (kernel)   | Begins as a simple I/O console via stdio; transitions to VMEXIT-based `IN/OUT` emulation. |
| Programmable Interrupt Controller (PIC) | VTXCTL (user space)  | VTXDRV (kernel)   | Initially mocked or bypassed; full interrupt injection moves to VTXDRV. |
| Programmable Interval Timer (PIT) | VTXCTL (timer thread) | VTXDRV (hardware timers) | Emulated via host timer loop early on; later replaced by kernel timer + VMEXIT-based IRQ0. |

## Limitations: Single-Core Guest Architecture

In the current design of VirtX, the guest virtual machine will operate as a **single-core system**. This design decision is informed by the simplicity of our emulated hardware environment:

- Only **one Programmable Interrupt Controller (PIC)** is emulated. Multi-core x86 systems rely on more complex interrupt routing via the Local APIC or IOAPIC, which are not part of this initial hypervisor implementation.
- Only **one Programmable Interval Timer (PIT)** is provided. Multiple timers and APIC timer management would be required for a symmetric multiprocessing (SMP) guest.
- The virtualization stack is designed to run a **single vCPU per guest**, which simplifies VMCS management, interrupt routing, and context switching.

As a result, guest operating systems (like NanoX or Linux) will perceive and utilize **only one CPU core**, even if the host hardware has multiple cores available. This constraint significantly reduces the complexity of emulation and interrupt handling, making the learning process more approachable and deterministic.

## Deliverable
An architectural understanding document jointly reviewed and internalized by both parent and child participants. This becomes the baseline for all subsequent projects and defines the ownership model for each component in the series.
