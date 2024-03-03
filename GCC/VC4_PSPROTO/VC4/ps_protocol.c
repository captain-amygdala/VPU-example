#include "ps_protocol.h"

/*
MBOX0 = ADDR
MBOX1 = DATAL
MBOX2 = DATAH
MBOX3 = CSR

CSR BITS:

0  START/BSY
1  R/_W
2  SIZE (8/16/32/64)
3  SIZE
4  STATUS_OP
.
.
.
*/

#define cmd_w8  1
#define cmd_r8  3
#define cmd_w16 5
#define cmd_r16 7
#define cmd_w32 9
#define cmd_r32 11
#define cmd_w64 13
#define cmd_r64 15
#define cmd_stat 17
#define cmd_ipl 19

int main (void){

 gpio = (unsigned int*)0x7e200000;
 mbox = (unsigned int*)0x7e0000a0;
 idle = 0xffffec;

 *(gdir0) = GPFSEL0_INPUT;*(gdir1) = GPFSEL1_INPUT;*(gdir2) = GPFSEL2_INPUT;

 unsigned int cmd;

 //asm("di");

 for(;;) {

  cmd = *(mbox3);
  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}

  switch(cmd & 0x1f) {
	case cmd_r8: ps_read_8(*(mbox0));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_r16:ps_read_16(*(mbox0));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_r32:ps_read_32(*(mbox0));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_r64:ps_read_32(*(mbox0));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_w8: ps_write_8(*(mbox0),*(mbox1));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_w16:ps_write_16(*(mbox0),*(mbox1));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_w32:ps_write_32(*(mbox0),*(mbox1));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_w64:ps_write_32(*(mbox0),*(mbox1));*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_stat:ps_read_status_reg();*(mbox3) = cmd &= ~(1 << 0);break;
        case cmd_ipl:*(mbox1) = *(gplev) & (1 << PIN_IPL_ZERO);*(mbox3) = cmd &= ~(1 << 0);break;
	default: break;
  }
 }
}

void ps_read_status_reg() {
  *(gset) = (REG_STATUS << PIN_A0);
  *(gset) = 1 << PIN_RD;*(gset) = 1 << PIN_RD;*(gset) = 1 << PIN_RD;*(gset) = 1 << PIN_RD;

  *(mbox1) = (*(gplev) >> 8) & 0xffff;
  *(gclr) = idle;
}

//WRITE 68k

void ps_write_8(unsigned int address, unsigned int data) {

  if ((address & 1) == 0)
    data = data + (data << 8);  // EVEN, A0=0,UDS
  else
    data = data & 0xff;  // ODD , A0=1,LDS

  *(gdir0) = GPFSEL0_OUTPUT;*(gdir1) = GPFSEL1_OUTPUT;*(gdir2) = GPFSEL2_OUTPUT;

  *(gset) = ((data & 0xffff) << 8) | (REG_DATA << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((address & 0xffff) << 8) | (REG_ADDR_LO << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((0x0100 | (address >> 16)) << 8) | (REG_ADDR_HI << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gdir0) = GPFSEL0_INPUT;*(gdir1) = GPFSEL1_INPUT;*(gdir2) = GPFSEL2_INPUT;

 // while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
}

void ps_write_16(unsigned int address, unsigned int data) {

  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}

  *(gdir0) = GPFSEL0_OUTPUT;*(gdir1) = GPFSEL1_OUTPUT;*(gdir2) = GPFSEL2_OUTPUT;

  *(gset) = ((data & 0xffff) << 8) | (REG_DATA << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((address & 0xffff) << 8) | (REG_ADDR_LO << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((0x0000 | (address >> 16)) << 8) | (REG_ADDR_HI << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gdir0) = GPFSEL0_INPUT;*(gdir1) = GPFSEL1_INPUT;*(gdir2) = GPFSEL2_INPUT;

 // while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
}

void ps_write_32(unsigned int address, unsigned int data) {
  ps_write_16(address, data >> 16);
  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
  ps_write_16(address + 2, data);
}


//READ 68k

void ps_read_8(unsigned int address) {
  *(gdir0) = GPFSEL0_OUTPUT;*(gdir1) = GPFSEL1_OUTPUT;*(gdir2) = GPFSEL2_OUTPUT;

  *(gset) = ((address & 0xffff) << 8) | (REG_ADDR_LO << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((0x0300 | (address >> 16)) << 8) | (REG_ADDR_HI << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gdir0) = GPFSEL0_INPUT;*(gdir1) = GPFSEL1_INPUT;*(gdir2) = GPFSEL2_INPUT;

  *(gset) = (REG_DATA << PIN_A0);*(gset) = 1 << PIN_RD;

//TODO
  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
  unsigned int value = *(gplev);

  *(gclr) = idle;

  value = (value >> 8) & 0xffff;

  if ((address & 1) == 0)
    *(mbox1) = (value >> 8) & 0xff;  // EVEN, A0=0,UDS
  else
    *(mbox1) = value & 0xff;  // ODD , A0=1,LDS
}
void ps_read_16(unsigned int address) {
  *(gdir0) = GPFSEL0_OUTPUT;*(gdir1) = GPFSEL1_OUTPUT;*(gdir2) = GPFSEL2_OUTPUT;

  *(gset) = ((address & 0xffff) << 8) | (REG_ADDR_LO << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gset) = ((0x0200 | (address >> 16)) << 8) | (REG_ADDR_HI << PIN_A0);
  *(gset) = 1 << PIN_WR;*(gclr) = 1 << PIN_WR;*(gclr) = idle;

  *(gdir0) = GPFSEL0_INPUT;*(gdir1) = GPFSEL1_INPUT;*(gdir2) = GPFSEL2_INPUT;

  *(gset) = (REG_DATA << PIN_A0);*(gset) = 1 << PIN_RD;

//TODO
  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
  unsigned int value = *(gplev);

  *(gclr) = idle;
  *(mbox1) = (value >> 8) & 0xffff;
}

void ps_read_32(unsigned int address) {
  unsigned int a;
  unsigned int b;

  ps_read_16(address);
  a = *(mbox1);
  while (*(gplev) & (1 << PIN_TXN_IN_PROGRESS)) {}
  ps_read_16(address + 2);
  b = *(mbox1);

  *(mbox1) = (a << 16) | b;
}





