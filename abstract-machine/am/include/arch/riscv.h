#ifndef ARCH_H__
#define ARCH_H__

#ifdef __riscv_e
#define NR_REGS 16
#else
#define NR_REGS 32
#endif

struct Context {
  union {
    // PA4之前，不涉及Context的切换，sp不会变。所以不用保存、恢复gpr[sp]
    // PA4.1之后，sp将恢复为__am_irq_handle的返回值（切换进程，恢复另一个进程的Context）。所以不用保存、恢复gpr[sp]
    // PA4.4之后，要实现用户栈和内核栈的切换，需要在进入内核栈之前，用gpr[sp]保存用户进程栈的地址，以便将来让sp恢复回到到用户进程栈
    uintptr_t gpr[NR_REGS];

    // 将地址空间信息与0号寄存器共用存储空间, 反正0号寄存器的值总是0, 也不需要保存和恢复
    void *pdir; // page directory 页目录表（一级页表）的地址
  };
  uintptr_t mcause, mstatus, mepc;
};

#ifdef __riscv_e
#define GPR1 gpr[15] // a5
#else
#define GPR1 gpr[17] // a7
#endif

#define GPR2 gpr[10] // a0
#define GPR3 gpr[11] // a1
#define GPR4 gpr[12] // a2
#define GPRx gpr[10] // a0

#endif
