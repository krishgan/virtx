# How to Set Up the Toolchain for Building vtxdrv

To build the `vtxdrv` kernel module, you need a working Linux development environment with the appropriate kernel headers and basic build tools installed.

## Step-by-Step Setup

### 1. Install Essential Build Tools
On Debian/Ubuntu:
```bash
sudo apt update
sudo apt install build-essential gcc make binutils
```

On Fedora/RHEL:
```bash
sudo dnf groupinstall "Development Tools"
sudo dnf install gcc make binutils
```

### 2. Install Kernel Headers
On Ubuntu:
```bash
sudo apt install linux-headers-$(uname -r)
```

On Fedora:
```bash
sudo dnf install kernel-devel
```

### 3. Verify Toolchain
Check if the following commands are available:
```bash
gcc --version
make --version
ld --version
```

### 4. (Optional) Useful Tools
- `dmesg`: To check kernel logs
- `modinfo`: To inspect module metadata
- `lsmod`: To list currently loaded modules
- `insmod`, `rmmod`: To load and unload modules

## Notes
- Ensure Secure Boot is disabled if you’re working on bare-metal hardware.
- If using a VM, ensure nested virtualization (VMX) is enabled.
