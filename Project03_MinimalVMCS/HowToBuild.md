# How to Build vtxdrv

## Requirements
- Linux kernel headers installed (for your current kernel)
- GCC, make
- Root access to load the kernel module
- VT-x capable Intel processor with VMX enabled in BIOS

## Steps

1. Build the kernel module:
    ```bash
    make
    ```

2. Load the module:
    ```bash
    sudo insmod vtxdrv.ko
    ```

3. Check dmesg output:
    ```bash
    dmesg | grep vtxdrv
    ```

4. To remove the module:
    ```bash
    sudo rmmod vtxdrv
    ```

5. Clean the build:
    ```bash
    make clean
    ```
