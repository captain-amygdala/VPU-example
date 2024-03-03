#include "ps_fastmem.h"

#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ps_autoconfig.h"
#include "ps_mappings.h"
#include "ps_protocol.h"
#include "psconf.h"

unsigned char fastmem[FASTMEM_SIZE];

#if !FASTMEM_FASTPATH
static unsigned int fastmem_read_8(unsigned int address) {
  return fastmem[address - FASTMEM_BASE];
}

static unsigned int fastmem_read_16(unsigned int address) {
  return be16toh(*(uint16_t *)&fastmem[address - FASTMEM_BASE]);
}

static unsigned int fastmem_read_32(unsigned int address) {
  return be32toh(*(uint32_t *)&fastmem[address - FASTMEM_BASE]);
}

static void fastmem_write_8(unsigned int address, unsigned int value) {
  fastmem[address - FASTMEM_BASE] = value;
}

static void fastmem_write_16(unsigned int address, unsigned int value) {
  *(uint16_t *)&fastmem[address - FASTMEM_BASE] = htobe16(value);
}

static void fastmem_write_32(unsigned int address, unsigned int value) {
  *(uint32_t *)&fastmem[address - FASTMEM_BASE] = htobe32(value);
}
#endif

static unsigned char ac_rom[] = {
    0xe, AC_MEM_SIZE_8MB,                   // 00/02, link into memory free list, 8 MB
    0x6, 0x9,                               // 04/06, product id
    0x8, 0x0,                               // 08/0a, preference to 8 MB space
    0x0, 0x0,                               // 0c/0e, reserved
    0x0, 0x7, 0xd, 0xb,                     // 10/12/14/16, mfg id
    0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x2, 0x0  // 18/.../26, serial
};

static void done_callback(int configured, unsigned int base) {
  if (!configured) {
    printf("Error: autoconfig did not map fastmem\n");
    exit(-1);
  }

  if (base != FASTMEM_BASE) {
    printf("Error: autoconfig mapped fastmem at address %08x, should be %08x\n",
           base, FASTMEM_BASE);
    exit(-1);
  }
}

void init_fastmem() {
#if !FASTMEM_FASTPATH
  struct ps_device fastmem_device = {
      fastmem_read_8, fastmem_read_16, fastmem_read_32,
      fastmem_write_8, fastmem_write_16, fastmem_write_32};

  unsigned int devno = ps_add_device(&fastmem_device);
  ps_add_range(devno, FASTMEM_BASE, FASTMEM_SIZE);
#endif

#if FASTMEM_AUTOCONFIG
  add_autoconfig_pic(ac_rom, sizeof(ac_rom), done_callback);
#endif
}
