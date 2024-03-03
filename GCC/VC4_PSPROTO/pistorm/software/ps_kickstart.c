#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <endian.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ps_mappings.h"
#include "psconf.h"

static uint8_t kickstart[KICK_SIZE];

static int load_kick_rom() {
  int fd = open("kick.rom", O_RDONLY);
  if (fd < 1) {
    printf("Failed loading kick.rom, using motherboard kickstart\n");
    return -1;
  }

  int size = (int)lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);

  int bytes_read = (int)read(fd, &kickstart, size);
  close(fd);

  if (bytes_read != size) {
    printf("Failed loading kick.rom, using motherboard kickstart\n");
    return -2;
  }

  if (size == 256 * 1024) {
    memcpy(&kickstart[size], &kickstart[0], size);
  }

  printf("Loaded kick.rom with size %d kib\n", size / 1024);
  return 0;
}

static unsigned int ks_read_8(unsigned int address) {
  return kickstart[address - KICK_BASE];
}

static unsigned int ks_read_16(unsigned int address) {
  return be16toh(*(uint16_t *)&kickstart[address - KICK_BASE]);
}

static unsigned int ks_read_32(unsigned int address) {
  return be32toh(*(uint32_t *)&kickstart[address - KICK_BASE]);
}

static void ks_write_fail(unsigned int address, unsigned int value) {
  printf("Warning: ignored write to kickstart, address=%08x, value=%08x\n", address, value);
}

int init_kickstart() {
  if (load_kick_rom() < 0)
    return -1;

  struct ps_device ks_device = {
      ks_read_8, ks_read_16, ks_read_32,
      ks_write_fail, ks_write_fail, ks_write_fail};

  uint32_t devno = ps_add_device(&ks_device);
  ps_add_range(devno, KICK_BASE, KICK_SIZE);
  return 0;
}
