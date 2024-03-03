#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include "mailbox.h"


#define BUS_TO_PHYS(x) ((x)&~0xC0000000)
#define CODESIZE 16384

int mailbox;
unsigned handle;
unsigned ptr;
uint8_t *program;


#define PIN_TXN_IN_PROGRESS 0
#define PIN_IPL_ZERO 1
#define PIN_A0 2
#define PIN_A1 3
#define PIN_CLK 4
#define PIN_UNUSED 5
#define PIN_RD 6
#define PIN_WR 7
#define PIN_D(x) (8 + x)

#define REG_DATA 0
#define REG_ADDR_LO 1
#define REG_ADDR_HI 2
#define REG_STATUS 3

#define STATUS_BIT_INIT 1
#define STATUS_BIT_RESET 2

#define STATUS_MASK_IPL 0xe000
#define STATUS_SHIFT_IPL 13

//#define BCM2708_PERI_BASE 0x20000000  // pi0-1
//#define BCM2708_PERI_BASE	0xFE000000  // pi4
#define BCM2708_PERI_BASE 0x3F000000  // pi3
#define BCM2708_PERI_SIZE 0x00F00000

#define GPIO_ADDR 0x200000 /* GPIO controller */
#define GPCLK_ADDR 0x101000

#define GPIO_BASE (BCM2708_PERI_BASE + 0x200000) /* GPIO controller */
#define GPCLK_BASE (BCM2708_PERI_BASE + 0x101000)

#define CLK_PASSWD 0x5a000000
#define CLK_GP0_CTL 0x070
#define CLK_GP0_DIV 0x074

// GPIO setup macros. Always use INP_GPIO(x) before using OUT_GPIO(x) or
// SET_GPIO_ALT(x,y)
#define INP_GPIO(g) *(gpio + ((g) / 10)) &= ~(7 << (((g) % 10) * 3))
#define OUT_GPIO(g) *(gpio + ((g) / 10)) |= (1 << (((g) % 10) * 3))
#define SET_GPIO_ALT(g, a)  \
  *(gpio + (((g) / 10))) |= \
      (((a) <= 3 ? (a) + 4 : (a) == 4 ? 3 : 2) << (((g) % 10) * 3))

#define GPIO_PULL *(gpio + 37)      // Pull up/pull down
#define GPIO_PULLCLK0 *(gpio + 38)  // Pull up/pull down clock

#define GPFSEL0_INPUT 0x0024c240
#define GPFSEL1_INPUT 0x00000000
#define GPFSEL2_INPUT 0x00000000

#define GPFSEL0_OUTPUT 0x0924c240
#define GPFSEL1_OUTPUT 0x09249249
#define GPFSEL2_OUTPUT 0x00000249

volatile unsigned int *gpio;
volatile unsigned int *gpclk;

//register volatile unsigned int *mbox asm ("r28");
volatile unsigned int *mbox;
volatile unsigned int *mbox3;

#define MBOX_ADDR 0x000000a0
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

unsigned int gpfsel0;
unsigned int gpfsel1;
unsigned int gpfsel2;

unsigned int gpfsel0_o;
unsigned int gpfsel1_o;
unsigned int gpfsel2_o;

static void setup_io() {
  int fd = open("/dev/mem", O_RDWR | O_SYNC);
  if (fd < 0) {
    printf("Unable to open /dev/mem. Run as root using sudo?\n");
    exit(-1);
  }

  void *gpio_map = mmap(
      NULL,                    // Any adddress in our space will do
      BCM2708_PERI_SIZE,       // Map length
      PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
      MAP_SHARED,              // Shared with other processes
      fd,                      // File to map
      BCM2708_PERI_BASE        // Offset to GPIO peripheral
  );

  //close(fd);

  if (gpio_map == MAP_FAILED) {
    printf("gpio mmap failed, errno = %d\n", errno);
    exit(-1);
  }

  gpio = ((volatile unsigned *)gpio_map) + GPIO_ADDR / 4;
  gpclk = ((volatile unsigned *)gpio_map) + GPCLK_ADDR / 4;
  //mbox = ((volatile unsigned *)gpio_map) + MBOX_ADDR / 4;


  void *mbox_map = mmap(
      NULL,                    // Any adddress in our space will do
      1024,       // Map length
      PROT_READ | PROT_WRITE,  // Enable reading & writting to mapped memory
      MAP_SHARED,              // Shared with other processes
      fd,                      // File to map
      BCM2708_PERI_BASE        // Offset to GPIO peripheral
  );

  close(fd);

  if (mbox_map == MAP_FAILED) {
    printf("mbox mmap failed, errno = %d\n", errno);
    exit(-1);
  }
 mbox = ((volatile unsigned *)mbox_map) + MBOX_ADDR / 4;


    mailbox = mbox_open();
    handle = mem_alloc(mailbox, CODESIZE, 8, 4);
    ptr = mem_lock(mailbox, handle);
    program = mapmem(BUS_TO_PHYS(ptr), CODESIZE);

    int mfd = 0;
    mfd = open("code.bin", O_RDONLY);
    if (mfd < 1) {
     printf("Failed loading code.bin\n");
    exit(0);
    } else {
     int size = (int)lseek(mfd, 0, SEEK_END);
     if (size > CODESIZE){
        printf("VPU binary exceeds maximum codesize\n");
        exit(0);
        }
     lseek(mfd, 0, SEEK_SET);
     read(mfd, program, size);
     printf("Loaded VPU1 code.bin with size %d byte\n", size);
    }

    printf("Execute VPU1, return R0 = 0x%08x\n",  execute_vpu1(mailbox, ptr, 0, 0, 0, 0, 0, 0));

}

static void setup_gpclk() {
  // Enable 200MHz CLK output on GPIO4, adjust divider and pll source depending
  // on pi model
  *(gpclk + (CLK_GP0_CTL / 4)) = CLK_PASSWD | (1 << 5);
  usleep(10);
  while ((*(gpclk + (CLK_GP0_CTL / 4))) & (1 << 7))
    ;
  usleep(100);
  *(gpclk + (CLK_GP0_DIV / 4)) =
      CLK_PASSWD | (6 << 12);  // divider , 6=200MHz on pi3
  usleep(10);
  *(gpclk + (CLK_GP0_CTL / 4)) =
      CLK_PASSWD | 5 | (1 << 4);  // pll? 6=plld, 5=pllc
  usleep(10);
  while (((*(gpclk + (CLK_GP0_CTL / 4))) & (1 << 7)) == 0)
    ;
  usleep(100);

  SET_GPIO_ALT(PIN_CLK, 0);  // gpclk0
}

void ps_setup_protocol() {
  setup_io();
  setup_gpclk();

  *(gpio + 10) = 0xffffec;

  *(gpio + 0) = GPFSEL0_INPUT;
  *(gpio + 1) = GPFSEL1_INPUT;
  *(gpio + 2) = GPFSEL2_INPUT;
}

void ps_write_8(unsigned int address, unsigned int data) {
 *(mbox + 0) = address;
 *(mbox + 1) = data;
 *(mbox + 3) = cmd_w8;
  while (*(mbox+3) & (1 << 0))
    ;
}
void ps_write_16(unsigned int address, unsigned int data) {

 *(mbox + 0) = address;
 *(mbox + 1) = data;
 *(mbox + 3) = cmd_w16;
  while (*(mbox+3) & (1 << 0))
    ;
}

/*
void ps_write_32(unsigned int address, unsigned int value) {
  ps_write_16(address, value >> 16);
  ps_write_16(address + 2, value);
}
*/


void ps_write_32(unsigned int address, unsigned int data) {
 *(mbox + 0) = address;
 *(mbox + 1) = data;
 *(mbox + 3) = cmd_w32;
  while (*(mbox + 3) & (1 << 0))
    ;
}


unsigned int ps_read_8(unsigned int address) {
 *(mbox + 0) = address;
 *(mbox + 3) = cmd_r8;
  while (*(mbox+3) & (1 << 0))
    ;
  return *(mbox + 1);
}

unsigned int ps_read_16(unsigned int address) {
 *(mbox + 0) = address;
 *(mbox + 3) = cmd_r16;
  while (*(mbox+3) & (1 << 0))
    ;
  return *(mbox + 1);
}


/*
unsigned int ps_read_32(unsigned int address) {
  unsigned int a = ps_read_16(address);
  unsigned int b = ps_read_16(address + 2);
  return (a << 16) | b;
}
*/


unsigned int ps_read_32(unsigned int address) {
 *(mbox + 0) = address;
 *(mbox + 3) = cmd_r32;
  while (*(mbox + 3) & (1 << 0))
    ;
  return *(mbox + 1);
}


void ps_write_status_reg(unsigned int value) {
  *(gpio + 0) = GPFSEL0_OUTPUT;
  *(gpio + 1) = GPFSEL1_OUTPUT;
  *(gpio + 2) = GPFSEL2_OUTPUT;

  *(gpio + 7) = ((value & 0xffff) << 8) | (REG_STATUS << PIN_A0);

  *(gpio + 7) = 1 << PIN_WR;
  *(gpio + 7) = 1 << PIN_WR;  // delay
  *(gpio + 10) = 1 << PIN_WR;
  *(gpio + 10) = 0xffffec;

  *(gpio + 0) = GPFSEL0_INPUT;
  *(gpio + 1) = GPFSEL1_INPUT;
  *(gpio + 2) = GPFSEL2_INPUT;
}


unsigned int ps_read_status_reg() {

 *(mbox + 3) = cmd_stat;
  while (*(mbox + 3) & (1 << 0))
    ;
  return *(mbox + 1);
}

void ps_reset_state_machine() {
  ps_write_status_reg(STATUS_BIT_INIT);
  usleep(1500);
  ps_write_status_reg(0);
  usleep(100);
}

void ps_pulse_reset() {
  ps_write_status_reg(0);
  usleep(100000);
  ps_write_status_reg(STATUS_BIT_RESET);
}

unsigned int ps_get_ipl_zero() {
 *(mbox + 3) = cmd_ipl;
  while (*(mbox + 3) & (1 << 0))
    ;
  return *(mbox + 1);
}
