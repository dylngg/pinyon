ENTRY(start)

/* Modified from https://wiki.osdev.org/ARMv7-A_Bare_Bones */
SECTIONS {
    /* After our kernel (non-task) stack; See bootup.S */
    . = 0xb000;
    __code_start = .;
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
    /* 16 byte alignment needed for efficient bss zeroing */
    . = ALIGN(16);
    __bss_start = .;
    .bss : {
        *(.bss COMMON)
    }
    . = ALIGN(16);
    __bss_end = .;
    __code_end = .;
}
