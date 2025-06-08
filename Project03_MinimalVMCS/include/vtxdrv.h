#ifndef VTXDRV_H
#define VTXDRV_H

#include <linux/ioctl.h>

#define DEVICE_NAME "vtx"
#define VTX_IOCTL_CREATE_VM _IOW('v', 1, struct vtx_vm_config)

struct vtx_vm_config {
    char name[32];
    unsigned long guest_mem_size;
};

#endif