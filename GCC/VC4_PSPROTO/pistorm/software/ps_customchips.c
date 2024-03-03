#include <stdint.h>

#include "a314/a314.h"
#include "gayle.h"
#include "m68k.h"
#include "ps_mappings.h"
#include "ps_protocol.h"
#include "psconf.h"

#define INTENAR 0xdff01c
#define INTREQR 0xdff01e
#define INTENA 0xdff09a
#define INTREQ 0xdff09c

#define INTF_SETCLR 0x8000
#define INTF_INTEN 0x4000
#define INTF_PORTS 0x0008

static unsigned int intena_shadow = 0;

#define INT2_ENABLED ((intena_shadow & (INTF_INTEN | INTF_PORTS)) == (INTF_INTEN | INTF_PORTS))

static unsigned int emu_int2_req() {
  return check_gayle_irq()
#if A314_ENABLED
         || a314_check_int2()
#endif
      ;
}

static unsigned int read_intenar() {
  unsigned int value = ps_read_16(INTENAR);
  intena_shadow = value;
  return value;
}

static unsigned int read_intreqr() {
  unsigned int value = ps_read_16(INTREQR);

  if (emu_int2_req())
    value |= INTF_PORTS;

  return value;
}

static void write_intena(unsigned int value) {
  ps_write_16(INTENA, value);

  if (value & INTF_SETCLR)
    intena_shadow |= value & (~INTF_SETCLR);
  else
    intena_shadow &= ~value;
}

void ps_update_irq() {
  unsigned int ipl = 0;

  if (!ps_get_ipl_zero()) {
    unsigned int status = ps_read_status_reg();
    ipl = (status & 0xe000) >> 13;
  }

  if (ipl < 2 && INT2_ENABLED && emu_int2_req()) {
    ipl = 2;
  }

  m68k_set_irq(ipl);
}

static unsigned int cc_read_8(unsigned int address) {
  return ps_read_8(address);
}

static unsigned int cc_read_16(unsigned int address) {
  if (address == INTENAR)
    return read_intenar();
  else if (address == INTREQR)
    return read_intreqr();
  else
    return ps_read_16(address);
}

static unsigned int cc_read_32(unsigned int address) {
  unsigned int a = cc_read_16(address);
  unsigned int b = cc_read_16(address + 2);
  return (a << 16) | b;
}

static void cc_write_8(unsigned int address, unsigned int value) {
  ps_write_8(address, value);
}

static void cc_write_16(unsigned int address, unsigned int value) {
  if (address == INTENA)
    write_intena(value);
  else
    ps_write_16(address, value);
}

static void cc_write_32(unsigned int address, unsigned int value) {
  cc_write_16(address, value >> 16);
  cc_write_16(address + 2, value);
}

int init_customchips() {
  struct ps_device cc_device = {
      cc_read_8, cc_read_16, cc_read_32,
      cc_write_8, cc_write_16, cc_write_32};

  uint32_t devno = ps_add_device(&cc_device);
  ps_add_range(devno, 0xdf0000, 0x10000);
  return 0;
}
