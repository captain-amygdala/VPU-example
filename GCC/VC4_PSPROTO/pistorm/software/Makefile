EXENAME          = pistorm

MAINFILES        = pistorm.c ps_protocol.c ps_mappings.c ps_autoconfig.c ps_customchips.c ps_fastmem.c ps_kickstart.c gayle.c ide/ide.c mailbox.c
MUSASHIFILES     = m68kcpu.c softfloat/softfloat.c 
MUSASHIGENCFILES = m68kops.c
MUSASHIGENHFILES = m68kops.h
MUSASHIGENERATOR = m68kmake

EXEPATH = ./

.CFILES   = $(MAINFILES) $(MUSASHIFILES) $(MUSASHIGENCFILES)
.OFILES   = $(.CFILES:%.c=%.o) a314/a314.o

CC        = gcc
CPP       = g++
WARNINGS  = -Wall -Wextra -pedantic
CFLAGS    = $(WARNINGS) -march=armv7 -O3
#CFLAGS    = $(WARNINGS) -O3
LFLAGS    = $(WARNINGS)

TARGET = $(EXENAME)$(EXE)

DELETEFILES = $(MUSASHIGENCFILES) $(MUSASHIGENHFILES) $(.OFILES) $(TARGET) $(MUSASHIGENERATOR)$(EXE)

all: $(TARGET)

clean:
	rm -f $(DELETEFILES)

$(TARGET): $(MUSASHIGENHFILES) $(.OFILES) Makefile
	$(CC) -o $@ $(.OFILES) -O3 -pthread $(LFLAGS) -lm -lstdc++

a314/a314.o: a314/a314.cc a314/a314.h
	$(CPP) -c -o a314/a314.o -O3 a314/a314.cc

$(MUSASHIGENCFILES) $(MUSASHIGENHFILES): $(MUSASHIGENERATOR)$(EXE)
	$(EXEPATH)$(MUSASHIGENERATOR)$(EXE)

$(MUSASHIGENERATOR)$(EXE):  $(MUSASHIGENERATOR).c
	$(CC) -o  $(MUSASHIGENERATOR)$(EXE)  $(MUSASHIGENERATOR).c
