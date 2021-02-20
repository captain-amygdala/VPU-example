C = main.c mailbox.c
H = mailbox.h

F = -lrt -lm -ldl

all:	vcrun code

vcrun:	$(C) $(H)
	gcc -o vcrun $(F) $(C) $(H)

code:
	./vasmvidcore -Fbin -o code.bin code.asm
clean:
	rm -f vcrun
	rm -f code.bin
