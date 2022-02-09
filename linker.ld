ENTRY(start)

/* Modified from https://wiki.osdev.org/ARMv7-A_Bare_Bones */
SECTIONS {
    /* After our kernel (non-task) stack; See bootup.S */
    . = 0xb000;
    .text : {
        *(.boot)
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    .data : {
        *(.data)
    }
    __bss_start = .;
    .bss : {
        *(.bss COMMON)
    }
    __bss_end = .;
}
