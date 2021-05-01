CC=arm-none-eabi-g++
OBJDIR=obj
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -nostdlib
INCLUDE=-I.
CXXFLAGS=-Wall -Wextra -std=c++14 -g
PINE_OBJ=$(OBJDIR)/pine/badmath.o $(OBJDIR)/pine/string.o
KERNEL_OBJ=$(OBJDIR)/kernel.o $(OBJDIR)/kmalloc.o $(OBJDIR)/console.o $(OBJDIR)/interrupts.o $(OBJDIR)/timer.o $(OBJDIR)/tasks.o $(OBJDIR)/shell.o $(OBJDIR)/lib.o
KERNEL_ASM_OBJ=$(OBJDIR)/bootup.o $(OBJDIR)/vector.o $(OBJDIR)/switch.o $(OBJDIR)/syscall.o

.PHONY: all
all: pinyon.elf

# Pine: the shared Userspace and Kernel library
pine: $(OBJDIR) $(PINE_OBJ)

pinyon.elf: $(OBJDIR) pine $(KERNEL_ASM_OBJ) $(KERNEL_OBJ)
	$(CC) -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(KERNEL_ASM_OBJ) $(KERNEL_OBJ) $(PINE_OBJ)

.PHONY: fmt
fmt:
	clang-format -i -style=WebKit *.cpp *.hpp pine/*.hpp pine/*.cpp

.PHONY: run
run: pinyon.elf
	# FIXME: Get a newer QEMU that supports raspi3
	qemu-system-arm -m 1024 -M raspi2 -serial stdio -kernel pinyon.elf

.PHONY: debug
debug: pinyon.elf
	# FIXME: Get a newer QEMU that supports raspi3
	qemu-system-arm -s -S -nographic -m 1G -M raspi2 -kernel pinyon.elf 1>pinyon.out &
	sleep 1  # hack
	arm-none-eabi-gdb pinyon.elf
	killall qemu-system-arm

.PHONY: clean
clean:
	rm -rf obj/ pinyon.elf pinyon.out

$(OBJDIR)/%.o: %.cpp
	$(CC) $(ARCHFLAGS) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) $(ARCHFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)/pine
	mkdir -p $(OBJDIR)
