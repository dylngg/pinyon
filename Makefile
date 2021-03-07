.POSIX:
OBJDIR=obj/
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -nostdlib
CFLAGS=-Wall -Wextra

all: kernel
kernel:
	mkdir -p $(OBJDIR)
	arm-none-eabi-g++ $(ARCHFLAGS) $(CLFAGS) -c kernel.cpp -o $(OBJDIR)/kernel.o
	arm-none-eabi-g++ $(ARCHFLAGS) -c bootup.S -o $(OBJDIR)/bootup.o
	arm-none-eabi-g++ -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(OBJDIR)/bootup.o $(OBJDIR)/kernel.o
run: all
	# FIXME: Get a newer QEMU that supports raspi3
	qemu-system-arm -m 1024 -M raspi2 -serial stdio -kernel pinyon.elf
clean:
	rm -rf obj/ pinyon.elf
