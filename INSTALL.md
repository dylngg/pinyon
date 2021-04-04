# Install Guide

The following guide assumes a MacOS machine with MacPorts and a XCode toolchain.

## Prequesites

```bash
port install arm-none-eabi-binutils   # @2.34_1
port install arm-none-eabi-gcc        # @9.2.0_1
port install arm-none-eabi-gdb        # @7.9
port install qemu +target_arm +usb +cocoa
```
