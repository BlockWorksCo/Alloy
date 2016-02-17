



SECTIONS
{
    .text :
    {
        *(.text)
        *(.text.*)
    }

    .rodata :
    {
        *(.rodata)
        *(.rodata1)
        *(.rodata.*)
    }

    .data :
    {
        *(.data)
        *(.data1)
        *(.data.*)
    }

    .bss :
    {
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        *(COMMON)
    }
}
