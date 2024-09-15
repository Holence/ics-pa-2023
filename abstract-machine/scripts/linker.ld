ENTRY(_start) /* entry point is the symbol _start */
PHDRS { text PT_LOAD; data PT_LOAD; } /* program headers segment, one loadable segment for executable code and one loadable segment for data */

SECTIONS {
  /* _pmem_start and _entry_offset are defined in LDFLAGS */
  
  . = _pmem_start + _entry_offset; /* .表示这里，内存从地址为 _pmem_start+_entry_offset 的地方开始 */
  
  .text : {
    *(entry) /* 就是 start.S 中的 _start 函数 */
    *(.text*)
  } : text
  etext = .;
  _etext = .;
  .rodata : {
    *(.rodata*)
  }
  .data : {
    *(.data)
  } : data
  edata = .;
  _data = .;
  .bss : {
	_bss_start = .;
    *(.bss*)
    *(.sbss*)
    *(.scommon)
  }
  _stack_top = ALIGN(0x1000);
  . = _stack_top + 0x8000;
  _stack_pointer = .; /* .表示这里，_stack_pointer为当前这里的地址 */
  end = .;
  _end = .;
  _heap_start = ALIGN(0x1000);
}
