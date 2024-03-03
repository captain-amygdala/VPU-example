/*
 * Copyright 2020 Claude Schwarz
 *
 * Niklas Ekström 2020 - reorganized source code
 */
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "a314/a314.h"
#include "gayle.h"
#include "m68k.h"
#include "ps_autoconfig.h"
#include "ps_customchips.h"
#include "ps_fastmem.h"
#include "ps_kickstart.h"
#include "ps_mappings.h"
#include "ps_protocol.h"
#include "psconf.h"

int use_gayle_emulation;

static void parse_args(int argc, char *argv[]) {
  use_gayle_emulation = 1;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--disable-gayle") == 0) {
      use_gayle_emulation = 0;
    }
  }
}

int main(int argc, char *argv[]) {
  printf("PiStorm 68k accelerator\n");
  printf("Copyright 2020 Claude Schwarz\n");

  parse_args(argc, argv);

  const struct sched_param priority = {99};
  sched_setscheduler(0, SCHED_FIFO, &priority);
  mlockall(MCL_CURRENT);

  init_mappings();
  init_autoconfig();
  init_fastmem();
  init_customchips();

  ps_setup_protocol();
  ps_reset_state_machine();
  ps_pulse_reset();

  usleep(1500);

  m68k_init();
  m68k_set_cpu_type(M68K_CPU_TYPE_68000);
  m68k_pulse_reset();

  int res = init_kickstart();
  if (res == 0)
    m68k_set_reg(M68K_REG_PC, KICK_BASE + 2);

  if (use_gayle_emulation)
    init_gayle("hd0.img");

#if A314_ENABLED
  res = a314_init();
  if (res < 0) {
    printf("Unable to initialize A314 emulation\n");
  }
#endif

  while (1) {
    ps_update_irq();
    m68k_execute(1000);
  }

  return 0;
}

void cpu_pulse_reset() {
  ps_pulse_reset();
}
