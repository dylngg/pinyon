# This makefile is not quite POSIX compatable and relies on GNU make features,
# but it works with the old MacOS GNU make that comes by default with XCode
# developer tools.
CC=arm-none-eabi-g++
HOST_CC=c++
# compiledb: Alternatively, 'python3 -m compiledb' works when installed via
#            'pip3 install --user compiledb'; this command generates a
#            compile_commands.json file from our Makefile, which IDEs such as
#            VS Code use. Not required for building.
COMPILEDB=python3 -m compiledb
QEMU=qemu-system-arm
QEMU_FLAGS=-m 1G -M raspi2
OBJDIR=obj
HOSTOBJDIR=obj/host
# -fno-threadsafe-statics: for static initialization (T& getT() { static Type t {}; return t; })
#                          don't produce thread-safe initialization of statics (normally required);
#                          this static getter is used to get around the
#                          https://isocpp.org/wiki/faq/ctors#static-init-order problem.
ARCH_LDFLAGS=-lgcc
ARCHFLAGS=-mcpu=cortex-a7 -fpic -fno-exceptions -ffreestanding -fno-threadsafe-statics -nostdlib -fno-rtti -fno-use-cxa-atexit
INCLUDE=-I. -Ipine
UBSAN_FLAGS=-fsanitize=undefined
ARCH_UBSAN_FLAGS=$(UBSAN_FLAGS) -fsanitize-undefined-trap-on-error
HOST_UBSAN_FLAGS=$(UBSAN_FLAGS) # -fsanitize=unsigned-integer-overflow -fsanitize=integer  # clang only
# -Wconversion: Yes, it complains a lot about unnecessary stuff; but it is
#               also the only tool to catch real issues here...
CXXFLAGS=-Wall -Wextra -Wpedantic -Wconversion -std=c++20 -g2 -O2
PINE_OBJ=$(OBJDIR)/pine/c_string.o $(OBJDIR)/pine/malloc.o $(OBJDIR)/pine/print.o $(OBJDIR)/pine/c_builtins.o
PINE_HOST_OBJ=$(HOSTOBJDIR)/pine/c_string.o $(HOSTOBJDIR)/pine/malloc.o $(HOSTOBJDIR)/pine/print.o $(HOSTOBJDIR)/pine/c_builtins.o
KERNEL_OBJ=$(OBJDIR)/kernel/console.o $(OBJDIR)/kernel/display.o $(OBJDIR)/kernel/file.o $(OBJDIR)/kernel/interrupts.o $(OBJDIR)/kernel/kernel.o $(OBJDIR)/kernel/kmalloc.o $(OBJDIR)/kernel/mailbox.o $(OBJDIR)/kernel/mmu.o $(OBJDIR)/kernel/processor.o $(OBJDIR)/kernel/stack.o $(OBJDIR)/kernel/tasks.o $(OBJDIR)/kernel/timer.o $(OBJDIR)/kernel/uart.o
KERNEL_ASM_OBJ=$(OBJDIR)/kernel/bootup.o $(OBJDIR)/kernel/switch.o $(OBJDIR)/kernel/vector.o
USER_OBJ=$(OBJDIR)/userspace/shell.o $(OBJDIR)/userspace/lib.o
USER_ASM_OBJ=$(OBJDIR)/userspace/syscall.o
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
	$(CC) -T linker.ld -o pinyon.elf $(ARCHFLAGS) $(KERNEL_ASM_OBJ) $(KERNEL_OBJ) $(USER_OBJ) $(USER_ASM_OBJ) $(PINE_OBJ) $(ARCH_LDFLAGS)

.PHONY: fmt
fmt:
	git diff --name-only | grep '\.\(cpp\|hpp\)$$' | xargs clang-format -i -style=WebKit

.PHONY: fullfmt
fullfmt:
	clang-format -i -style=WebKit kernel/*.cpp kernel/*.hpp pine/*.hpp pine/*.cpp userspace/*.hpp userspace/*.cpp

.PHONY: run
run: pinyon.elf
	qemu-system-arm $(QEMU_FLAGS) -serial stdio -kernel $<

.PHONY: debug
debug: pinyon.elf
	sh feed.sh | qemu-system-arm -s -S -nographic $(QEMU_FLAGS) -kernel $< 1>pinyon.out 2>&1 &
	sleep 1  # hack
	arm-none-eabi-gdb $<
	killall qemu-system-arm

.PHONY: trace
trace: pinyon.elf
	# FIXME: Get a newer QEMU that supports raspi3
	sh feed.sh | qemu-system-arm -s -S -nographic $(QEMU_FLAGS) -d mmu,exec,guest_errors,cpu,in_asm,plugin,page -kernel $< 1>pinyon.out 2>&1 &
	sleep 1  # hack
	arm-none-eabi-gdb $<
	killall qemu-system-arm

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
	$(CC) $(ARCHFLAGS) $(INCLUDE) $(CXXFLAGS) $(ARCH_UBSAN_FLAGS) -c $< -o $@

$(HOSTOBJDIR)/%.o: %.cpp
	$(HOST_CC) $(INCLUDE) $(CXXFLAGS) $(HOST_UBSAN_FLAGS) -c $< -o $@

$(OBJDIR)/%.o: %.S
	$(CC) $(ARCHFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)/kernel
	mkdir -p $(OBJDIR)/userspace
	mkdir -p $(OBJDIR)/pine

$(HOSTOBJDIR): $(OBJDIR)
	mkdir -p $(HOSTOBJDIR)/pine
