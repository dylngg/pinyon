CC=arm-none-eabi-g++
OBJDIR=obj
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -nostdlib
INCLUDE=-I.
CXXFLAGS=-Wall -Wextra -std=c++14 -g

.PHONY: all
all: pinyon.elf

# Pine: the shared Userspace and Kernel library
pine: $(OBJDIR) $(OBJDIR)/pine/badmath.o $(OBJDIR)/pine/string.o $(OBJDIR)/pine/console.o

pinyon.elf: $(OBJDIR) pine $(OBJDIR)/kernel.o
	$(CC) $(ARCHFLAGS) -c bootup.S -o $(OBJDIR)/bootup.o
	$(CC) -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(OBJDIR)/bootup.o $(OBJDIR)/kernel.o $(OBJDIR)/pine/badmath.o $(OBJDIR)/pine/console.o $(OBJDIR)/pine/string.o

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
	qemu-system-arm -s -S -nographic -m 1G -M raspi2 -kernel pinyon.elf 1>/dev/null &
	sleep 1  # hack
	arm-none-eabi-gdb pinyon.elf
	killall qemu-system-arm

.PHONY: clean
clean:
	rm -rf obj/ pinyon.elf

$(OBJDIR)/%.o: %.cpp
	$(CC) $(ARCHFLAGS) $(INCLUDE) $(CXXFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)/pine
	mkdir -p $(OBJDIR)
