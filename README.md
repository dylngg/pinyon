# Pinyon

Pinyon is a toy ARMv7 and AArch64 kernel targeting the Raspberry Pi 2 and 3. It presently has the following features:

- [x] Booting
- [x] UART console output
- [x] Basic UNIX-like shell
- [x] Preemptive round-robin scheduling
- [x] Memory allocation
- [x] `open()`/`read()`/`write()` syscalls for UART IO
- [x] Process CPU time and memory reporting
- [x] System uptime reporting
- [x] Virtual memory
- [x] Tasks run with non-supervisor privileges

## Prerequisites

GNU Make is required for building. On MacOS, the default `make` that comes with XCode developer tools seems to work.

For `AARCH64=0` (ARMv7) the ARM GNU toolchain is required. This can be obtained from [ARM's website](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-a/downloads).

If the GCC toolchain is downloaded from ARM, you will either need it in your `$PATH`.

```
$ export PATH="$PATH:/your/path/to/arm-none-eabi-gcc/bin"
```

Otherwise, for `AARCH64=1` Clang and LLVM is required. The default `clang++` that comes with XCode developer tools works for compilation, however, `llvm-objcopy` and `ld.lld` will have to be downloaded.

QEMU is required and be downloaded from [QEMU's website](https://www.qemu.org/download/#macos).

## Running

Building and running is straightforward:

```bash
make            # build aarch64
AARCH64=0 make  # build armv7
```

```
make run        # after building
```

## Using

The internal `help` command details what commands are available in Pinyon.

To exit, hit Ctrl-C. The `exit` command inside Pinyon does nothing.

## Developing

A `compile_commands.json` file, often used by editors such as VS Code, can be generated using [compiledb](https://github.com/nickdiego/compiledb). You might want to edit the `COMPILEDB` macro in the [Makefile](Makefile), depending on how you installed it.
