# Whether or not to build with Clang
CLANG ?= 1
# AARCH64 must be built with CLANG
AARCH64 ?= 1

ifeq ($(CLANG),1)
CC=clang++
else
CC=arm-none-eabi-g++
endif

ifeq ($(CLANG),1)
LD=ld.lld-mp-14
OBJCOPY=llvm-objcopy-mp-14
else
LD=arm-none-eabi-ld
OBJCOPY=arm-none-eabi-objcopy
endif

HOST_CC=g++
# compiledb: Alternatively, 'python3 -m compiledb' works when installed via
#            'pip3 install --user compiledb'; this command generates a
#            compile_commands.json file from our Makefile, which IDEs such as
#            VS Code use. Not required for building.
COMPILEDB=python3 -m compiledb

ifeq ($(AARCH64), 1)
QEMU=qemu-system-aarch64
QEMU_FLAGS=-m 1G -M raspi3b
else
QEMU=qemu-system-arm
QEMU_FLAGS=-m 1G -M raspi2
endif

ifeq ($(AARCH64), 1)
GDB=ggdb
else
GDB=arm-none-eabi-gdb
endif

OBJDIR=obj
HOSTOBJDIR=obj/host

ifeq ($(CLANG),1)

ifeq ($(AARCH64),1)
ARCHFLAGS=--target=aarch64-none-eabi -mcpu=cortex-a53+nofp+nosimd
else
ARCHFLAGS=--target=armv7-none-eabi -mcpu=cortex-a7+nofp+nosimd
endif

else
ARCHFLAGS=-mcpu=cortex-a7
endif

ifeq ($(CLANG),1)
DEFINES=-DCLANG_HAS_NO_CXX_INCLUDES
endif

# -fno-threadsafe-statics: for static initialization (T& getT() { static Type t {}; return t; })
#                          don't produce thread-safe initialization of statics (normally required);
#                          this static getter is used to get around the
#                          https://isocpp.org/wiki/faq/ctors#static-init-order problem.
FREESTANDING_FLAGS=-fpic -fno-exceptions -ffreestanding -fno-threadsafe-statics -nostdlib -fno-rtti -fno-use-cxa-atexit
INCLUDE=-I. -Ipine
UBSAN_FLAGS=-fsanitize=undefined
ARCH_UBSAN_FLAGS=$(UBSAN_FLAGS) -fsanitize-undefined-trap-on-error
HOST_UBSAN_FLAGS=$(UBSAN_FLAGS) # -fsanitize=unsigned-integer-overflow -fsanitize=integer  # clang only
# -Wconversion: Yes, it complains a lot about unnecessary stuff; but it is
#               also the only tool to catch real issues here...
# -Wno-volatile: This mistake by the C++ committe was undone in C++23.
CXXFLAGS=-Wall -Wextra -Wpedantic -Wconversion -Wno-volatile -std=c++20 -g2
PINE_OBJ=$(OBJDIR)/pine/c_string.o $(OBJDIR)/pine/malloc.o $(OBJDIR)/pine/print.o $(OBJDIR)/pine/c_builtins.o $(OBJDIR)/pine/arch/eabi.o
PINE_HOST_OBJ=$(HOSTOBJDIR)/pine/c_string.o $(HOSTOBJDIR)/pine/malloc.o $(HOSTOBJDIR)/pine/print.o $(HOSTOBJDIR)/pine/c_builtins.o

ifeq ($(AARCH64),1)
ARCH_DEFINES=-DAARCH64
KERNEL_ASM_OBJ=$(OBJDIR)/kernel/arch/aarch64/bootup.o $(OBJDIR)/kernel/arch/aarch64/vector.o
KERNEL_OBJ=$(OBJDIR)/kernel/console.o $(OBJDIR)/kernel/kernel.o $(OBJDIR)/kernel/device/bcm2835/timer.o $(OBJDIR)/kernel/device/pl011/uart.o
USER_OBJ=
USER_ASM_OBJ=
else
ARCH_DEFINES=-DAARCH32
KERNEL_OBJ=$(OBJDIR)/kernel/console.o $(OBJDIR)/kernel/device/bcm2835/display.o $(OBJDIR)/kernel/arch/aarch32/exception.o $(OBJDIR)/kernel/file.o $(OBJDIR)/kernel/device/bcm2835/interrupts.o $(OBJDIR)/kernel/kernel.o $(OBJDIR)/kernel/kmalloc.o $(OBJDIR)/kernel/device/videocore/mailbox.o $(OBJDIR)/kernel/arch/aarch32/mmu.o $(OBJDIR)/kernel/arch/aarch32/processor.o $(OBJDIR)/kernel/stack.o $(OBJDIR)/kernel/tasks.o $(OBJDIR)/kernel/device/bcm2835/timer.o $(OBJDIR)/kernel/device/pl011/uart.o
KERNEL_ASM_OBJ=$(OBJDIR)/kernel/arch/aarch32/bootup.o $(OBJDIR)/kernel/switch.o $(OBJDIR)/kernel/arch/aarch32/vector.o
USER_OBJ=$(OBJDIR)/userspace/shell.o $(OBJDIR)/userspace/lib.o
USER_ASM_OBJ=$(OBJDIR)/userspace/syscall.o
endif

TESTS=pine/test/twomath.hpp pine/test/twomath.hpp pine/test/maybe.hpp
TESTFILE=pine/test/test.cpp

.PHONY: all
all: pinyon.elf

# Pine: the shared Userspace and Kernel library
.PHONY: pine
pine: $(OBJDIR) $(PINE_OBJ)

.PHONY: pine_host
pine_host: $(HOSTOBJDIR) $(PINE_HOST_OBJ)

.PHONY: userspace
userspace: $(OBJDIR) $(USER_ASM_OBJ) $(USER_OBJ)

.PHONY: kernel
kernel: $(OBJDIR) $(KERNEL_ASM_OBJ) $(KERNEL_OBJ)

pinyon.elf: $(OBJDIR) pine userspace kernel
	$(LD) -T linker.ld -o pinyon.elf $(KERNEL_ASM_OBJ) $(KERNEL_OBJ) $(USER_OBJ) $(USER_ASM_OBJ) $(PINE_OBJ)

.PHONY: fmt
fmt:
	git diff --name-only | grep '\.\(cpp\|hpp\)$$' | xargs clang-format -i -style=WebKit

.PHONY: fullfmt
fullfmt:
	clang-format -i -style=WebKit kernel/*.cpp kernel/*.hpp pine/*.hpp pine/*.cpp userspace/*.hpp userspace/*.cpp

.PHONY: run
run: pinyon.elf
	$(QEMU) $(QEMU_FLAGS) -serial stdio -kernel $<

.PHONY: debug
debug: pinyon.elf
	sh feed.sh | $(QEMU) -s -S -nographic $(QEMU_FLAGS) -kernel $< 1>pinyon.out 2>&1 &
	sleep 1  # hack
	$(GDB) $<
	killall $(QEMU)

ifneq ($(AARCH64),1)
.PHONY: trace
trace: pinyon.elf
	sh feed.sh | $(QEMU) -s -S -nographic $(QEMU_FLAGS) -d mmu,exec,guest_errors,cpu,in_asm,plugin,page -kernel $< 1>pinyon.out 2>&1 &
	sleep 1  # hack
	$(GDB) $<
	killall $(QEMU)
endif

.PHONY: test
test: test_pine
	./test_pine

test_pine: pine_host $(TESTS) $(TESTFILE)
	$(HOST_CC) $(INCLUDE) $(CXXFLAGS) $(HOST_UBSAN_FLAGS) $(TESTFILE) $(PINE_HOST_OBJ) -o $@

.PHONY:
compile_commands.json: clean
	$(COMPILEDB) make

.PHONY:
tags:
	find pine/ kernel/ userspace/ -type f -name '*.hpp' -o -name '*.cpp' -print0 | xargs -0 ctags

.PHONY: clean
clean:
	rm -rf obj/ pinyon.elf pinyon.out

$(OBJDIR)/%.o: %.cpp
	$(CC) $(DEFINES) $(ARCH_DEFINES) $(ARCHFLAGS) $(FREESTANDING_FLAGS) $(INCLUDE) $(CXXFLAGS) $(ARCH_UBSAN_FLAGS) -c $< -o $@

$(HOSTOBJDIR)/%.o: %.cpp
	$(HOST_CC) $(INCLUDE) $(CXXFLAGS) $(HOST_UBSAN_FLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) $(ARCHFLAGS) $(FREESTANDING_FLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)/kernel
	mkdir -p $(OBJDIR)/kernel/arch/aarch64/
	mkdir -p $(OBJDIR)/kernel/arch/aarch32/
	mkdir -p $(OBJDIR)/kernel/device/bcm2835/
	mkdir -p $(OBJDIR)/kernel/device/videocore/
	mkdir -p $(OBJDIR)/kernel/device/pl011/
	mkdir -p $(OBJDIR)/userspace
	mkdir -p $(OBJDIR)/pine
	mkdir -p $(OBJDIR)/pine/arch

$(HOSTOBJDIR): $(OBJDIR)
	mkdir -p $(HOSTOBJDIR)/pine
