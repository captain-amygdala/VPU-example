#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include "mailbox.h"


#define BUS_TO_PHYS(x) ((x)&~0xC0000000)

#define CODESIZE 1024

int mbox;
unsigned handle;
unsigned ptr;
uint8_t *program;


void setup() {
    mbox = mbox_open();
    handle = mem_alloc(mbox, CODESIZE, 8, 4);
    ptr = mem_lock(mbox, handle);
    program = mapmem(BUS_TO_PHYS(ptr), CODESIZE);

    int fd = 0;
    fd = open("code.bin", O_RDONLY);
    if (fd < 1) {
     printf("Failed loading code.bin\n");
    exit(0);
    } else {
     int size = (int)lseek(fd, 0, SEEK_END);
     if (size > CODESIZE){
	printf("VPU binary exceeds maximum codesize\n");
	exit(0);
	}
     lseek(fd, 0, SEEK_SET);
     read(fd, program, size);
     printf("Loaded code.bin with size %d byte\n", size);
    }

}


void cleanup() {
    unmapmem(program, 64);
    mem_unlock(mbox, handle);
    mem_free(mbox, handle);
    mbox_close(mbox);
}


unsigned readmem(unsigned phys) {
    return execute_code(mbox, ptr, phys, 0, 0, 0, 0, 0);
}


void writemem(unsigned phys, unsigned value) {
    execute_code(mbox, ptr+4, phys, value, 0, 0, 0, 0);
}


int main(int argc, char *argv[]) {
    setup();
    printf("Start VPU code\n");
    printf("Returnvalue R0 = 0x%08x\n",  execute_code(mbox, ptr, 0, 0, 0, 0, 0, 0));
    cleanup();

    return 0;
}
