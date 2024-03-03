#include "ps_mappings.h"

#include <assert.h>
#include <endian.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ps_fastmem.h"
#include "ps_protocol.h"
#include "psconf.h"

static struct ps_device devices[MAX_MAPPING_DEVICES];
static int devices_size = 0;

#if USE_MAPPING_TABLE
static unsigned char range_mapping[256];
#endif

#if USE_MAPPING_LIST
struct mapping_list_entry {
  unsigned int start;
  unsigned int end;
  struct ps_device *device;
};

static struct mapping_list_entry mapping_list[MAX_MAPPING_LIST_LENGTH];
static int mapping_list_size = 0;
#endif

unsigned int m68k_read_memory_8(unsigned int address) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    return fastmem[address - FASTMEM_BASE];
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  return devices[devno].read_8(address);
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end)
      return e->device->read_8(address);
  }
  return ps_read_8(address);
#endif
}

unsigned int m68k_read_memory_16(unsigned int address) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    return be16toh(*(uint16_t *)&fastmem[address - FASTMEM_BASE]);
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  return devices[devno].read_16(address);
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end)
      return e->device->read_16(address);
  }
  return ps_read_16(address);
#endif
}

unsigned int m68k_read_memory_32(unsigned int address) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    return be32toh(*(uint32_t *)&fastmem[address - FASTMEM_BASE]);
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  return devices[devno].read_32(address);
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end)
      return e->device->read_32(address);
  }
  return ps_read_32(address);
#endif
}

void m68k_write_memory_8(unsigned int address, unsigned int value) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    fastmem[address - FASTMEM_BASE] = value;
    return;
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  devices[devno].write_8(address, value);
  return;
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end) {
      e->device->write_8(address, value);
      return;
    }
  }
  ps_write_8(address, value);
#endif
}

void m68k_write_memory_16(unsigned int address, unsigned int value) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    *(uint16_t *)&fastmem[address - FASTMEM_BASE] = htobe16(value);
    return;
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  devices[devno].write_16(address, value);
  return;
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end) {
      e->device->write_16(address, value);
      return;
    }
  }
  ps_write_16(address, value);
#endif
}

void m68k_write_memory_32(unsigned int address, unsigned int value) {
#if FASTMEM_FASTPATH
  if (address >= FASTMEM_BASE && address < FASTMEM_BASE + FASTMEM_SIZE) {
    *(uint32_t *)&fastmem[address - FASTMEM_BASE] = htobe32(value);
    return;
  }
#endif

#if USE_MAPPING_TABLE
  unsigned int devno = range_mapping[(address >> 16) & 0xff];
  devices[devno].write_32(address, value);
  return;
#endif

#if USE_MAPPING_LIST
  for (int i = 0; i < mapping_list_size; i++) {
    struct mapping_list_entry *e = &mapping_list[i];
    if (address >= e->start && address <= e->end) {
      e->device->write_32(address, value);
      return;
    }
  }
  ps_write_32(address, value);
#endif
}

int ps_add_device(struct ps_device *device) {
  if (devices_size == MAX_MAPPING_DEVICES) {
    printf("Error: Too many mapped devices, max = %d\n", MAX_MAPPING_DEVICES);
    exit(-1);
  }

  int devno = devices_size;
  devices_size++;

  memcpy(&devices[devno], device, sizeof(struct ps_device));
  return devno;
}

void ps_add_range(unsigned char devno, unsigned int base, unsigned int size) {
  if ((base & 0xffff) != 0 || (size & 0xffff) != 0) {
    printf("Error: mappings must be 64 kB aligned\n");
    exit(-1);
  }

  if (size == 0) {
    printf("Error: mappings cannot be zero size\n");
    exit(-1);
  }

#if USE_MAPPING_TABLE
  int start = base >> 16;
  int count = size >> 16;

  if (start > 255 || start + count > 256) {
    printf("Error: mapping table can only use the first 16 MB\n");
    exit(-1);
  }

  for (int i = 0; i < count; i++)
    range_mapping[start + i] = devno;
#endif

#if USE_MAPPING_LIST
  if (mapping_list_size == MAX_MAPPING_LIST_LENGTH) {
    printf("Error: Out of mapping list entries, max=%d\n", MAX_MAPPING_LIST_LENGTH);
    exit(-1);
  }

  struct mapping_list_entry *entry = &mapping_list[mapping_list_size];
  entry->start = base;
  entry->end = base + size - 1;
  entry->device = &devices[devno];
  mapping_list_size++;
#endif
}

void init_mappings() {
  struct ps_device mobo_device = {
      ps_read_8, ps_read_16, ps_read_32,
      ps_write_8, ps_write_16, ps_write_32};

  uint32_t devno = ps_add_device(&mobo_device);
  assert(devno == 0);

#if USE_MAPPING_TABLE
  memset(range_mapping, 0, sizeof(range_mapping));
#endif
}
