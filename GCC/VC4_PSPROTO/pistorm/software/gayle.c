//
//  Gayle.c
//  Omega
//
//  Created by Matt Parsons on 06/03/2019.
//  Copyright © 2019 Matt Parsons. All rights reserved.
//
//  Changes made 2020 by Niklas Ekström to better fit PiStorm.

// Write Byte to Gayle Space 0xda9000 (0x0000c3)
// Read Byte From Gayle Space 0xda9000
// Read Byte From Gayle Space 0xdaa000

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ide/ide.h"
#include "ps_mappings.h"

#define CLOCKBASE 0xDC0000

//#define GSTATUS 0xda201c
//#define GCLOW   0xda2010
//#define GDH	0xda2018

// Gayle Addresses

// Gayle IDE Reads
#define GERROR 0xda2004   // Error
#define GSTATUS 0xda201c  // Status
// Gayle IDE Writes
#define GFEAT 0xda2004  // Write : Feature
#define GCMD 0xda201c   // Write : Command
// Gayle IDE RW
#define GDATA 0xda2000     // Data
#define GSECTCNT 0xda2008  // SectorCount
#define GSECTNUM 0xda200c  // SectorNumber
#define GCYLLOW 0xda2010   // CylinderLow
#define GCYLHIGH 0xda2014  // CylinderHigh
#define GDEVHEAD 0xda2018  // Device/Head
#define GCTRL 0xda3018     // Control
// Gayle Ident
#define GIDENT 0xDE1000

// Gayle IRQ/CC
#define GCS 0xDA8000   // Card Control
#define GIRQ 0xDA9000  // IRQ
#define GINT 0xDAA000  // Int enable
#define GCONF 0xDAB000  // Gayle Config

/* DA8000 */
#define GAYLE_CS_IDE 0x80   /* IDE int status */
#define GAYLE_CS_CCDET 0x40 /* credit card detect */
#define GAYLE_CS_BVD1 0x20  /* battery voltage detect 1 */
#define GAYLE_CS_SC 0x20    /* credit card status change */
#define GAYLE_CS_BVD2 0x10  /* battery voltage detect 2 */
#define GAYLE_CS_DA 0x10    /* digital audio */
#define GAYLE_CS_WR 0x08    /* write enable (1 == enabled) */
#define GAYLE_CS_BSY 0x04   /* credit card busy */
#define GAYLE_CS_IRQ 0x04   /* interrupt request */
#define GAYLE_CS_DAEN 0x02  /* enable digital audio */
#define GAYLE_CS_DIS 0x01   /* disable PCMCIA slot */

/* DA9000 */
#define GAYLE_IRQ_IDE 0x80
#define GAYLE_IRQ_CCDET 0x40 /* credit card detect */
#define GAYLE_IRQ_BVD1 0x20  /* battery voltage detect 1 */
#define GAYLE_IRQ_SC 0x20    /* credit card status change */
#define GAYLE_IRQ_BVD2 0x10  /* battery voltage detect 2 */
#define GAYLE_IRQ_DA 0x10    /* digital audio */
#define GAYLE_IRQ_WR 0x08    /* write enable (1 == enabled) */
#define GAYLE_IRQ_BSY 0x04   /* credit card busy */
#define GAYLE_IRQ_IRQ 0x04   /* interrupt request */
#define GAYLE_IRQ_RESET 0x02 /* reset machine after CCDET change */
#define GAYLE_IRQ_BERR 0x01  /* generate bus error after CCDET change */

/* DAA000 */
#define GAYLE_INT_IDE 0x80     /* IDE interrupt enable */
#define GAYLE_INT_CCDET 0x40   /* credit card detect change enable */
#define GAYLE_INT_BVD1 0x20    /* battery voltage detect 1 change enable */
#define GAYLE_INT_SC 0x20      /* credit card status change enable */
#define GAYLE_INT_BVD2 0x10    /* battery voltage detect 2 change enable */
#define GAYLE_INT_DA 0x10      /* digital audio change enable */
#define GAYLE_INT_WR 0x08      /* write enable change enabled */
#define GAYLE_INT_BSY 0x04     /* credit card busy */
#define GAYLE_INT_IRQ 0x04     /* credit card interrupt request */
#define GAYLE_INT_BVD_LEV 0x02 /* BVD int level, 0=lev2,1=lev6 */
#define GAYLE_INT_BSY_LEV 0x01 /* BSY int level, 0=lev2,1=lev6 */

static int counter;
static uint8_t gayle_irq;
static uint8_t gayle_int;
static uint8_t gayle_cs;
static uint8_t gayle_cs_mask;
static uint8_t gayle_cfg;
static struct ide_controller *ide0;
static int fd;

static void gayle_write_8(unsigned int address, unsigned int value) {
  switch (address) {
    case GFEAT:
      ide_write8(ide0, ide_feature_w, value);
      break;

    case GCMD:
      ide_write8(ide0, ide_command_w, value);
      break;

    case GSECTCNT:
      ide_write8(ide0, ide_sec_count, value);
      break;

    case GSECTNUM:
      ide_write8(ide0, ide_sec_num, value);
      break;

    case GCYLLOW:
      ide_write8(ide0, ide_cyl_low, value);
      break;

    case GCYLHIGH:
      ide_write8(ide0, ide_cyl_hi, value);
      break;

    case GDEVHEAD:
      ide_write8(ide0, ide_dev_head, value);
      break;

    case GCTRL:
      ide_write8(ide0, ide_devctrl_w, value);
      break;

    case GIDENT:
      counter = 0;
      // printf("Write Byte to Gayle Ident 0x%06x (0x%06x)\n",address,value);
      break;

    case GIRQ:
      //	 printf("Write Byte to Gayle GIRQ 0x%06x (0x%06x)\n",address,value);
      gayle_irq = (gayle_irq & value) | (value & (GAYLE_IRQ_RESET | GAYLE_IRQ_BERR));
      break;

    case GCS:
      printf("Write Byte to Gayle GCS 0x%06x (0x%06x)\n", address, value);
      gayle_cs_mask = value & ~3;
      gayle_cs &= ~3;
      gayle_cs |= value & 3;
      break;

    case GINT:
      printf("Write Byte to Gayle GINT 0x%06x (0x%06x)\n", address, value);
      gayle_int = value;
      break;

    case GCONF:
      printf("Write Byte to Gayle GCONF 0x%06x (0x%06x)\n", address, value);
      gayle_cfg = value;
      break;

    default:
      printf("Write Byte to Gayle Space 0x%06x (0x%06x)\n", address, value);
      break;
  }
}

static void gayle_write_16(unsigned int address, unsigned int value) {
  switch (address) {
    case GDATA:
      ide_write16(ide0, ide_data, value);
      break;

    default:
      printf("Write Word to Gayle Space 0x%06x (0x%06x)\n", address, value);
      break;
  }
}

static void gayle_write_32(unsigned int address, unsigned int value) {
  switch (address) {
    default:
      printf("Write Long to Gayle Space 0x%06x (0x%06x)\n", address, value);
      break;
  }
}

static unsigned int gayle_read_8(unsigned int address) {
  switch (address) {
    case GERROR:
      return ide_read8(ide0, ide_error_r);

    case GSTATUS:
      return ide_read8(ide0, ide_status_r);

    case GSECTCNT:
      return ide_read8(ide0, ide_sec_count);

    case GSECTNUM:
      return ide_read8(ide0, ide_sec_num);

    case GCYLLOW:
      return ide_read8(ide0, ide_cyl_low);

    case GCYLHIGH:
      return ide_read8(ide0, ide_cyl_hi);

    case GDEVHEAD:
      return ide_read8(ide0, ide_dev_head);

    case GCTRL:
      return ide_read8(ide0, ide_altst_r);

    case GIDENT: {
      uint8_t val;
      // printf("Read Byte from Gayle Ident 0x%06x (0x%06x)\n",address,counter);
      if (counter == 0 || counter == 1 || counter == 3) {
        val = 0x80;  // 80; to enable gayle
      } else {
        val = 0x00;
      }
      counter++;
      return val;
    }

    case GIRQ:
      //	printf("Read Byte From GIRQ Space 0x%06x\n",gayle_irq);
      if (ide0->drive->intrq) {
        // printf("IDE IRQ: %x\n",irq);
        return 0x80;  // gayle_irq;
      }
      return 0;

    case GCS: {
      printf("Read Byte From GCS Space 0x%06x\n", 0x1234);
      uint8_t v;
      v = gayle_cs_mask | gayle_cs;
      return v;
    }

    case GINT:
      //	printf("Read Byte From GINT Space 0x%06x\n",gayle_int);
      return gayle_int;

    case GCONF:
      printf("Read Byte From GCONF Space 0x%06x\n", gayle_cfg & 0x0f);
      return gayle_cfg & 0x0f;

    default:
      printf("Read Byte From Gayle Space 0x%06x\n", address);
      return 0xFF;
  }
}

static unsigned int gayle_read_16(unsigned int address) {
  switch (address) {
    case GDATA: {
      uint16_t value;
      value = ide_read16(ide0, ide_data);
      //	value = (value << 8) | (value >> 8);
      return value;
    }

    default:
      printf("Read Word From Gayle Space 0x%06x\n", address);
      return 0x8000;
  }
}

static unsigned int gayle_read_32(unsigned int address) {
  switch (address) {
    default:
      printf("Read Long From Gayle Space 0x%06x\n", address);
      return 0x8000;
  }
}

unsigned int check_gayle_irq() {
  if (gayle_int & (1 << 7))
    return ide0->drive->intrq;

  return 0;
}

int init_gayle(const char image_name[]) {
  ide0 = ide_allocate("cf");

  fd = open(image_name, O_RDWR);
  if (fd < 0) {
    printf("HDD Image %s failed open\n", image_name);
    return -1;
  }

  ide_attach(ide0, 0, fd);
  ide_reset_begin(ide0);
  printf("HDD Image %s attached\n", image_name);

  struct ps_device gayle_device = {
      gayle_read_8, gayle_read_16, gayle_read_32,
      gayle_write_8, gayle_write_16, gayle_write_32};

  uint32_t devno = ps_add_device(&gayle_device);
  ps_add_range(devno, 0xda0000, 0x10000);
  ps_add_range(devno, 0xde0000, 0x10000);
  return 0;
}
