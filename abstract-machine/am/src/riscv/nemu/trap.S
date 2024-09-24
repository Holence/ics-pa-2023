#define concat_temp(x, y) x ## y
#define concat(x, y) concat_temp(x, y)
#define MAP(c, f) c(f)

#if __riscv_xlen == 32
#define LOAD  lw
#define STORE sw
#define XLEN  4
#else
#define LOAD  ld
#define STORE sd
#define XLEN  8
#endif

#define REGS_LO16(f) \
      f( 1)       f( 3) f( 4) f( 5) f( 6) f( 7) f( 8) f( 9) \
f(10) f(11) f(12) f(13) f(14) f(15)

#ifndef __riscv_e
#define REGS_HI16(f) \
                                    f(16) f(17) f(18) f(19) \
f(20) f(21) f(22) f(23) f(24) f(25) f(26) f(27) f(28) f(29) \
f(30) f(31)
#define NR_REGS 32
#else
#define REGS_HI16(f)
#define NR_REGS 16
#endif

#define REGS(f) REGS_LO16(f) REGS_HI16(f)

#define PUSH(n) STORE concat(x, n), (n * XLEN)(sp);
#define POP(n)  LOAD  concat(x, n), (n * XLEN)(sp);

#define CONTEXT_SIZE  ((NR_REGS + 3 + 1) * XLEN) // gpr[32], mcause, mstatus, mepc, np
#define OFFSET_SP     ( 2 * XLEN)
#define OFFSET_CAUSE  ((NR_REGS + 0) * XLEN)
#define OFFSET_STATUS ((NR_REGS + 1) * XLEN)
#define OFFSET_EPC    ((NR_REGS + 2) * XLEN)
#define OFFSET_NP     ((NR_REGS + 3) * XLEN)

#define NP_KERNEL 0
#define NP_USER 1

.align 3
.globl __am_asm_trap

__am_asm_trap:
  
  // (1) atomically exchange sp and mscratch
  // 从mscratch读出ksp，将sp存入mscratch，此时只有sp寄存器可以被随意使用（作为临时寄存器用于下面的条件判断）
  csrrw sp, mscratch, sp
  
  // (2) take the branch if we trapped from user
  // sp不为0，则表明sp读入了非零的ksp，说明是用户进程，现在sp在内核栈里，接下来去save_context即可
  bnez sp, save_context_user
  
  // (3) if we trapped from kernel, restore the original sp
  // sp为0,则表明sp读入了为零的ksp，说明是内核线程，现在从mscratch读回sp的值
  csrr sp, mscratch
  j save_context_kernel

  // 现在mscratch中存的是最开始sp的值（进入trap时的值，为进程/线程的栈顶），后面要把这个值保存到Context.sp中，并且在退出trap时让sp恢复这个值（回到原先的进程/线程的栈顶）

// 代码复制粘贴
save_context_user:
  // now sp is pointing to the kernel stack
  // save the context...

  addi sp, sp, -CONTEXT_SIZE

  MAP(REGS, PUSH)

  // gpr存完了，终于可以霍霍临时寄存器了
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  li t3, NP_USER
  STORE t3, OFFSET_NP(sp) // Context.next态 = 用户态

  j save_context_done

// 代码复制粘贴
save_context_kernel:
  // now sp is pointing to the kernel stack
  // save the context...

  addi sp, sp, -CONTEXT_SIZE

  MAP(REGS, PUSH)

  // gpr存完了，终于可以霍霍临时寄存器了
  csrr t0, mcause
  csrr t1, mstatus
  csrr t2, mepc

  STORE t0, OFFSET_CAUSE(sp)
  STORE t1, OFFSET_STATUS(sp)
  STORE t2, OFFSET_EPC(sp)

  li t3, NP_KERNEL
  STORE t3, OFFSET_NP(sp) // Context.next态 = 内核态

save_context_done:
  csrr t4, mscratch
  STORE t4, OFFSET_SP(sp) // Context.sp = $sp; 保存进入trap时sp的值（进程/线程栈顶的地址）

  csrw mscratch, x0 // ksp = 0;

  # set mstatus.MPRV to pass difftest
  li a0, (1 << 17)
  or t1, t1, a0
  csrw mstatus, t1

  mv a0, sp // 给__am_irq_handle传入Context*
  jal __am_irq_handle
  mv sp, a0 // PA4开始涉及进程切换，这里要更新sp指向__am_irq_handle返回的新Context*
  // PA4.4后 sp在内核栈顶


  LOAD t1, OFFSET_STATUS(sp)
  LOAD t2, OFFSET_EPC(sp)
  csrw mstatus, t1
  csrw mepc, t2

  // if (Context.next态 == 用户态) {
  //  ksp = $sp;
  // }
  // 这里要的sp值是恢复完Context之后，sp应该向上移动CONTEXT_SIZE之后的值
  // 但是因为恢复gpr之后就没法霍霍临时寄存器了，只能在这里用临时寄存器给csr传数值
  LOAD t3, OFFSET_NP(sp)
  beqz t3, restore_context
  addi t4, sp, CONTEXT_SIZE
  csrw mscratch, t4

restore_context:
  // 从这开始就要恢复gpr了，不可以霍霍临时寄存器了
  MAP(REGS, POP)

  // addi sp, sp, CONTEXT_SIZE // PA4.4之前 trap就发生在各自的进程栈里，sp加加减减就行了
  
  LOAD sp, OFFSET_SP(sp) // PA4.4之后 $sp = Context.sp; 从内核栈跳回原先进程/线程的栈顶
  
  mret
