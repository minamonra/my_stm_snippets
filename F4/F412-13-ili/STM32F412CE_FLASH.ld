ENTRY(Reset_Handler)

/* Конец RAM и начальный указатель стека (стек растет сверху вниз) */
_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size = 0x200;      /* Размер кучи */
_Min_Stack_Size = 0x400;     /* Размер стека */

MEMORY
{
  RAM    (xrw)    : ORIGIN = 0x20000000, LENGTH = 256K
  FLASH  (rx)     : ORIGIN = 0x08000000, LENGTH = 512K
}

SECTIONS
{
  /* Таблица векторов прерываний */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* Код программы */
  .text :
  {
    . = ALIGN(4);
    *(.text)           
    *(.text*)          
    *(.glue_7)         
    *(.glue_7t)        
    *(.eh_frame)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;        
  } >FLASH

  /* Константы и строки (Read-Only Data) */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)         
    *(.rodata*)        
    . = ALIGN(4);
  } >FLASH

  _sidata = LOADADDR(.data);

  /* Инициализированные переменные (копируются в RAM при старте) */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;        
    *(.data)           
    *(.data*)          
    . = ALIGN(4);
    _edata = .;        
  } >RAM AT >FLASH

  /* Неинициализированные переменные */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;         
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;         
    __bss_end__ = _ebss;
  } >RAM

  /* Разметка под стек и кучу */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  /* Отбрасываем ненужные стандартные библиотеки */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }
}