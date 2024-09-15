.section entry, "ax" // 当前section的名字叫entry，ax表示属性是allocatable, executable
.globl _start
.type _start, @function

_start:
  mv s0, zero
  la sp, _stack_pointer
  jal _trm_init
