#ifndef NEMU_H__
#define NEMU_H__

#include <klib-macros.h>

#include ISA_H // the macro `ISA_H` is defined in CFLAGS
               // it will be expanded as "x86/x86.h", "mips/mips32.h", ...

#if defined(__ISA_X86__)
#define nemu_trap(code) asm volatile("int3" : : "a"(code))
#elif defined(__ISA_MIPS32__)
#define nemu_trap(code) asm volatile("move $v0, %0; sdbbp" : : "r"(code))
#elif defined(__riscv)
// __riscv 是 predefined compiler macro
// 可以打印看看 riscv64-linux-gnu-gcc -dM -E - < /dev/null | grep riscv
// code is a numeric value to be put into a gpr (general purpose register), which will be automatic selected by gcc in compile time
// then it will move this value from this gpr to a0 register
// then ebreak
#define nemu_trap(code) asm volatile("mv a0, %0; ebreak" : : "r"(code))
#elif defined(__ISA_LOONGARCH32R__)
#define nemu_trap(code) asm volatile("move $a0, %0; break 0" : : "r"(code))
#elif
#error unsupported ISA __ISA__
#endif

#if defined(__ARCH_X86_NEMU)
#define DEVICE_BASE 0x0
#else
#define DEVICE_BASE 0xa0000000
#endif

#define MMIO_BASE 0xa0000000

#define SERIAL_PORT (DEVICE_BASE + 0x00003f8)
#define KBD_ADDR (DEVICE_BASE + 0x0000060)
#define RTC_ADDR (DEVICE_BASE + 0x0000048)
#define VGACTL_ADDR (DEVICE_BASE + 0x0000100)
#define AUDIO_ADDR (DEVICE_BASE + 0x0000200)
#define DISK_ADDR (DEVICE_BASE + 0x0000300)
#define FB_ADDR (MMIO_BASE + 0x1000000)
#define AUDIO_SBUF_ADDR (MMIO_BASE + 0x1200000)

extern char _pmem_start;              // == 0x8000 0000 == CONFIG_MBASE
#define PMEM_SIZE (128 * 1024 * 1024) // == 0x800 0000 == CONFIG_MSIZE
#define PMEM_END ((uintptr_t) & _pmem_start + PMEM_SIZE)

// 这里除了pmem空间外，还有三个外设的区间
// MMIO  RANGE [0xa0000000, 0xa0001000]
// FB    RANGE [0xa1000000, 0xa1200000]
// AUDIO RANGE [0xa1200000, 0xa1010000]
// 0x200000的等于2MB的空间，比800x600的VGA需要1.8MB的空间，400x300的VGA就不在话下了
// AUDIO设定是64KB，给0x10000刚好
#define NEMU_PADDR_SPACE                                 \
  RANGE(&_pmem_start, PMEM_END),                         \
      RANGE(FB_ADDR, FB_ADDR + 0x200000),                \
      RANGE(AUDIO_SBUF_ADDR, AUDIO_SBUF_ADDR + 0x10000), \
      RANGE(MMIO_BASE, MMIO_BASE + 0x1000) /* serial, rtc, screen, keyboard */

typedef uintptr_t PTE;

#define PGSIZE 4096

#endif
