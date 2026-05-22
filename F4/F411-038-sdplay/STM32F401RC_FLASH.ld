/* STM32F401RCT6: 256KB Flash, 64KB RAM */
ENTRY(Reset_Handler)

/* Определяем вершину стека как самый конец физического ОЗУ (64KB) */
_estack = 0x20000000 + 64K;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
    .isr_vector : {
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text : {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        . = ALIGN(4);
    } > FLASH

    .rodata : {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _etext = .;
    PROVIDE(etext = .);

    .data : AT (_etext) {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM
    _sidata = LOADADDR(.data);

    .bss : {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /* Куча удалена. Стек автоматически займет всё оставшееся свободное ОЗУ до конца памяти */

    /DISCARD/ : {
        *(.ARM.exidx)
        *(.ARM.exidx.*)
        *(.eh_frame)
        *(.eh_frame.*)
    }
}