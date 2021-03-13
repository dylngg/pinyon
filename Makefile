.POSIX:
CC=arm-none-eabi-g++
OBJDIR=obj
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -nostdlib
INCLUDE=-I.
CXXFLAGS=-Wall -Wextra -std=c++11

all: kernel

# Pine: the shared Userspace and Kernel library
pine: objdir pine/badmath.o pine/string.o pine/console.o

kernel: objdir pine
	$(CC) $(ARCHFLAGS) $(INCLUDE) $(CXXFLAGS) -c kernel.cpp -o $(OBJDIR)/kernel.o
	$(CC) $(ARCHFLAGS) -c bootup.S -o $(OBJDIR)/bootup.o
	$(CC) -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(OBJDIR)/bootup.o $(OBJDIR)/kernel.o $(OBJDIR)/pine/badmath.o $(OBJDIR)/pine/console.o $(OBJDIR)/pine/string.o

fmt:
	clang-format -i -style=WebKit *.cpp *.hpp pine/*.hpp pine/*.cpp

run: kernel
	# FIXME: Get a newer QEMU that supports raspi3
	qemu-system-arm -m 1024 -M raspi2 -serial stdio -kernel pinyon.elf

clean:
	rm -rf obj/ pinyon.elf

# Used by pine/*.o targets
.SUFFIXES: .cpp .o
.cpp.o:
	$(CC) $(ARCHFLAGS) $(INCLUDE) $(CXXFLAGS) -c $< -o $(OBJDIR)/$*.o

# Generic target to create $(OBJDIR)/
objdir:
	mkdir -p $(OBJDIR)/pine
	mkdir -p $(OBJDIR)
