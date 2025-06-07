# How to Install and Run vtxdrv

## Prerequisites
- Root (sudo) access
- Kernel headers installed (matching the running kernel)
- VMX (Intel VT-x) enabled in BIOS
- Ensure `dmesg` and `/dev` access

## Step-by-Step Guide

### 1. Build the Module
```bash
make
```

### 2. Insert the Kernel Module
```bash
sudo insmod vtxdrv.ko
```

### 3. Check Logs
```bash
dmesg | grep vtxdrv
```

### 4. Create Device Node (if later extended to support ioctl/syscalls)
```bash
sudo mknod /dev/vtxdrv c <major_number> 0
sudo chmod 666 /dev/vtxdrv
```
> Replace `<major_number>` with the number returned by `dmesg` or dynamically queried via `cat /proc/devices`.

**Note**: In the current VMX-only setup, no device file is required unless extended for user-kernel communication.

### 5. Remove the Module
```bash
sudo rmmod vtxdrv
```

### 6. Remove Device Node
```bash
sudo rm -f /dev/vtxdrv
```

## Verifying VMX
```bash
egrep --color 'vmx' /proc/cpuinfo
```

## Troubleshooting
- Make sure Secure Boot is disabled (some systems block VMX).
- VMX must be enabled in BIOS (check your BIOS settings).
- Ensure kernel headers are installed: `sudo apt install linux-headers-$(uname -r)`

