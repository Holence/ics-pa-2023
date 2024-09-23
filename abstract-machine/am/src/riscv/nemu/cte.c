#include <am.h>
#include <riscv/riscv.h>
#include <klib.h>

void __am_get_cur_as(Context *c);
void __am_switch(Context *c);

static Context *(*user_handler)(Event, Context *) = NULL;

Context *__am_irq_handle(Context *c) {
  // PA4
  void *c_before = c;
  // 在Context中保存之前进程的一级页表地址
  __am_get_cur_as(c);

  // 测试Context结构体定义的正确性
  // uintptr_t *p = c->gpr;
  // for (int i = 0; i < 32 + 3; i++) {
  //   printf("%d\n", *(p++));
  // }
  if (user_handler) {
    Event ev = {0};
    switch (c->mcause) {

    // ecall
    case EXP_MECALL:
      // 根据a7跳转
      if (c->GPR1 == 0xFFFFFFFF) {
        // yield 中赋值的 -1
        ev.event = EVENT_YIELD;
      } else {
        // syscall
        ev.event = EVENT_SYSCALL;
      }
      c->mepc += 4;
      break;

    case IRQ_MTIMER:
      ev.event = EVENT_IRQ_TIMER;
      // nemu里中断时给mepc的地址时dnpc，所以不用+4了
      break;

    default:
      ev.event = EVENT_ERROR;
      break;
    }

    // PA4后，user_handler可能会切换进程，返回另一个进程的context
    c = user_handler(ev, c);
    assert(c != NULL);
  }

  // PA4
  if (c != c_before) {
    // 从Context中取出一级页表地址，设置到satp中
    __am_switch(c);
  }

  return c;
}

extern void __am_asm_trap(void);

bool cte_init(Context *(*handler)(Event, Context *)) {
  // initialize exception entry
  asm volatile("csrw mtvec, %0" : : "r"(__am_asm_trap));
  // 实际编译出来是 csrw mtvec, a4（a4里面存着__am_asm_trap的地址）
  // 这串二进制为 0x30571073
  // 因为伪指令 CSRW csr, rs1 == CSRRW x0, csr, rs1
  // [   mtvec  ] [rs1] [funct3]  [rd] [opcode]
  // 001100000101 01110   001    00000 1110011
  // 说明在汇编中写mtvec这个名称的话，就会自动转换为这个csr寄存器的地址，也就是0x305

  // register event handler
  user_handler = handler;

  return true;
}

// 初始化进程，创建空白context
// - mepc（mret跳转到的地址）设置为进程的entry地址
// - a0设置为arg
Context *kcontext(Area kstack, void (*entry)(void *), void *arg) {
  Context *p = (Context *)(kstack.end - sizeof(Context));
  p->mepc = (uintptr_t)entry; // 设置mret将要跳转到entry
  p->GPR2 = (uintptr_t)arg;   // 设置即将传入entry的第一个参数a0的值为arg
  p->pdir = NULL;
  p->mstatus = 0x1800 | MSTATUS_MIE;
  return p;
}

void yield() {
#ifdef __riscv_e
  asm volatile("li a5, -1; ecall");
#else
  asm volatile("li a7, -1; ecall");
#endif
}

// 是否开中断
bool ienabled() {
  return false;
}

// 设置中断开关
void iset(bool enable) {
}
