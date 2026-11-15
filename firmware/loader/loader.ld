/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */

MEMORY
{
    LDR (rwx) : ORIGIN = 0x01C0A800, LENGTH = 0x14000
}
SECTIONS
{
    .text : {
        KEEP(*(.text.crt0)) *(.text*) *(.ram_text*) *(.rodata*) *(.data*) *(.sdata*)
        . = ALIGN(4); _image_end = .;
    } > LDR
    .bss (NOLOAD) : { _bss_start = .; *(.bss*) *(.sbss*) *(COMMON) . = ALIGN(4); _bss_end = .; } > LDR
    _stack_top = 0x01C70000;
    _sstack_top = 0x01C72000;
    /DISCARD/ : { *(.comment) *(.note*) *(.noinit*) }
}
