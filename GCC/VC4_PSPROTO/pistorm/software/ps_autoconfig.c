#include <stdio.h>
#include <stdlib.h>

#include "ps_mappings.h"
#include "psconf.h"

#define AUTOCONFIG_BASE 0xe80000
#define AUTOCONFIG_SIZE (64 * 1024)

struct autoconfig_pic {
  unsigned char *rom;
  unsigned int rom_size;
  int configured;
  unsigned int base;
  void (*done_callback)(int configured, unsigned int base);
};

static struct autoconfig_pic pics[MAX_AUTOCONFIG_PICS];
static int pic_count = 0;
static int current_pic = 0;

static unsigned int ac_read_memory_8(unsigned int address) {
  if (current_pic == pic_count) {
    return 0;
  }

  struct autoconfig_pic *pic = &pics[current_pic];
  address -= AUTOCONFIG_BASE;

  unsigned char val = 0;
  if ((address & 1) == 0 && (address / 2) < pic->rom_size)
    val = pic->rom[address / 2];
  val <<= 4;
  if (address != 0 && address != 2 && address != 40 && address != 42)
    val ^= 0xf0;
  return (unsigned int)val;
}

static void ac_write_memory_8(unsigned int address, unsigned int value) {
  if (current_pic == pic_count) {
    return;
  }

  struct autoconfig_pic *pic = &pics[current_pic];
  address -= AUTOCONFIG_BASE;
  int done = 0;

  if (address == 0x4a) {  // base[19:16]
    pic->base = (value & 0xf0) << (16 - 4);
  } else if (address == 0x48) {  // base[23:20]
    pic->base &= 0xff0fffff;
    pic->base |= (value & 0xf0) << (20 - 4);
    pic->configured = 1;
    done = 1;
  } else if (address == 0x4c) {  // shut up
    done = 1;
  }

  if (done) {
    pic->done_callback(pic->configured, pic->base);
    current_pic++;
  }
}

static unsigned int ac_ignore_read(__attribute__((unused)) unsigned int address) {
  return 0;
}

static void ac_ignore_write(__attribute__((unused)) unsigned int address, __attribute__((unused)) unsigned int value) {
}

void add_autoconfig_pic(unsigned char *rom, int rom_size, void (*done_callback)(int configured, unsigned int base)) {
  if (pic_count == MAX_AUTOCONFIG_PICS) {
    printf("Error: too many autoconfig PICs, max=%d\n", MAX_AUTOCONFIG_PICS);
    exit(-1);
  }

  int index = pic_count;
  pics[index].rom = rom;
  pics[index].rom_size = rom_size;
  pics[index].configured = 0;
  pics[index].base = 0;
  pics[index].done_callback = done_callback;
  pic_count++;
}

void init_autoconfig() {
  struct ps_device ac_device = {
      ac_read_memory_8, ac_ignore_read, ac_ignore_read,
      ac_write_memory_8, ac_ignore_write, ac_ignore_write};

  unsigned int devno = ps_add_device(&ac_device);
  ps_add_range(devno, AUTOCONFIG_BASE, AUTOCONFIG_SIZE);
}
