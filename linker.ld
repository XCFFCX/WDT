/* linker.ld — 明确控制各段地址 */
ENTRY(_start)

SECTIONS
{
    . = 0x40000000;

    .vectors : {
        KEEP(*(.vectors))
    }

    .text : {
        *(.text*)
    }

    .rodata : {
        *(.rodata*)
    }

    .data : {
        *(.data*)
    }

    .bss : {
        *(.bss*)
        *(COMMON)
    }
}