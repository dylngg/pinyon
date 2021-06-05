# Pinyon

Pinyon is a really basic toy ARMv7 kernel targeting the Raspberry Pi 2. It presently has the following features:

- [x] Booting
- [x] UART console output

and aims to support the following in the future:

- [x] Basic UNIX-like shell
- [x] Preemptive round-robin scheduling
- [x] Memory allocation
- [x] `readline()`/`write()` syscalls for UART IO
- [x] Process CPU time and memory reporting
- [x] System uptime reporting
- [ ] Virtual memory?!

## Prequesites

GNU Make is required for building. On MacOS, the default `make` seems to work.

You can obtain the ARM EABI GCC Toolchain with the following commands if using MacPorts on MacOS:

```bash
port install arm-none-eabi-binutils   # @2.34_1
port install arm-none-eabi-gcc        # @9.2.0_1
port install arm-none-eabi-gdb        # @7.9*
port install qemu +target_arm +usb +cocoa
```

Otherwise, the GCC toolchain can be obtained from [ARM's website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads) and QEMU from [QEMU's website](https://www.qemu.org/download/#macos).

If the GCC toolchain is downloaded from ARM, you will either need it in your `$PATH`, or you can edit the `CC` macro in the [Makefile](Makefile).

```
$ export PATH="$PATH:/your/path/to/arm-none-eabi-gcc/bin"
```

_\*Note: It appears that as of writing, the `arm-none-eabi-gdb` port in MacPorts does not build successfully. The `arm-none-eabi-gdb` binary in the GCC toolchain provided by ARM appears to work_

## Running

Building and running is straightforward:

```bash
make            # build
make run        # build and run
```

Note that the background emulator window is not used.

## Using

The internal `help` command details what commands are available in Pinyon.

To exit, hit Ctrl-C. The `exit` command inside Pinyon does nothing.
