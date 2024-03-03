./vc4-elf-as start.s -o start.o
#./vc4-elf-gcc -nostartfiles -fpic -ffreestanding -nostdlib ps_protocol.c -o ps_protocol.s -S
#./vc4-elf-as ps_protocol.s -o ps_protocol.bin -nostartfiles
#./vc4-elf-gcc -nostartfiles -fpic -ffreestanding -nostdlib start.o ps_protocol.c -O3 -o ps_protocol.bin
./vc4-elf-gcc -nostartfiles -fpic -nostdlib start.o ps_protocol.c -O3 -T link.ld -o ps_protocol.bin
./vc4-elf-objcopy -O binary ps_protocol.bin code.bin
scp code.bin pi@192.168.1.108:/home/pi/pistorm/software/code.bin
#scp code.bin pi@192.168.1.108:/home/pi/VPU-example/code.bin
#ssh -t pi@192.168.1.108 sudo reboot
