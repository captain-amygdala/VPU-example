C = main.c mailbox.c
H = mailbox.h

F = -lrt -lm -ldl

all:	vcpoke code

vcpoke:	$(C) $(H)
	gcc -o vcpoke $(F) $(C) $(H)

code:
	./vasmvidcore -Fbin -o code.bin code.asm
clean:
	rm -f vcpoke
	rm -f code.bin
