# Pinyon

Pinyon is a really basic toy ARMv7 kernel targeting the Raspberry Pi 2. Right now the project is in the early stages. It presently has the following features:

- [x] Booting
- [x] UART console output

and aims to support the following in the future:

- [ ] Basic UNIX-like shell 
- [ ] Preemptive round-robin scheduling
- [ ] Memory allocation
- [ ] `read()`/`write()` syscalls
- [ ] Process CPU time and memory reporting
- [ ] System uptime reporting
- [ ] Virtual memory?!

## Running

See [INSTALL.md](INSTALL.md) to install the requirements.

Building and running is straightforward:

```bash
make            # build
make run        # build and run
```

_Note: You may have to edit the `CC` macro in the [Makefile](Makefile) to point the build system to the location of your `arm-none-eabi-gcc` toolchain._
