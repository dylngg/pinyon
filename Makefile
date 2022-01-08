CC=arm-none-eabi-g++
OBJDIR=obj
# -fno-threadsafe-statics: for static initialization (T& getT() { static Type t {}; return t; })
#                          don't produce thread-safe initialization of statics (normally required);
#                          this static getter is used to get around the
#                          https://isocpp.org/wiki/faq/ctors#static-init-order problem.
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -fno-threadsafe-statics -nostdlib
INCLUDE=-I.
CXXFLAGS=-Wall -Wextra -Wpedantic -std=c++17 -g
PINE_OBJ=$(OBJDIR)/pine/badmath.o $(OBJDIR)/pine/string.o $(OBJDIR)/pine/malloc.o
KERNEL_OBJ=$(OBJDIR)/kernel/kernel.o $(OBJDIR)/kernel/kmalloc.o $(OBJDIR)/kernel/console.o $(OBJDIR)/kernel/interrupts.o $(OBJDIR)/kernel/timer.o $(OBJDIR)/kernel/tasks.o $(OBJDIR)/kernel/panic.o
KERNEL_ASM_OBJ=$(OBJDIR)/kernel/bootup.o $(OBJDIR)/kernel/vector.o $(OBJDIR)/kernel/switch.o
USER_OBJ=$(OBJDIR)/userspace/shell.o $(OBJDIR)/userspace/lib.o
USER_ASM_OBJ=$(OBJDIR)/userspace/syscall.o

.PHONY: all
all: pinyon.elf

# Pine: the shared Userspace and Kernel library
.PHONY: pine
pine: $(OBJDIR) $(PINE_OBJ)

.PHONY: userspace
userspace: $(OBJDIR) $(USER_ASM_OBJ) $(USER_OBJ)

.PHONY: kernel
kernel: $(OBJDIR) $(KERNEL_ASM_OBJ) $(KERNEL_OBJ)

pinyon.elf: $(OBJDIR) pine userspace kernel
	$(CC) -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(KERNEL_ASM_OBJ) $(KERNEL_OBJ) $(USER_OBJ) $(USER_ASM_OBJ) $(PINE_OBJ)

.PHONY: fmt
fmt:
	clang-format -i -style=WebKit kernel/*.cpp kernel/*.hpp pine/*.hpp pine/*.cpp userspace/*.hpp userspace/*.cpp

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
	mkdir -p $(OBJDIR)/kernel
	mkdir -p $(OBJDIR)/userspace
	mkdir -p $(OBJDIR)/pine
