#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../include/vtxdrv.h"

int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "--createvm") != 0) {
        fprintf(stderr, "Usage: %s --createvm <name>\n", argv[0]);
        return 1;
    }

    int fd = open("/dev/vtx", O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    struct vtx_vm_config config = {
        .guest_mem_size = 0x200000
    };
    strncpy(config.name, argv[2], sizeof(config.name) - 1);

    if (ioctl(fd, VTX_IOCTL_CREATE_VM, &config) != 0) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    printf("VM '%s' created.\n", config.name);
    close(fd);
    return 0;
}