#include <common.h>

void do_syscall(Context *c);
Context *schedule(Context *prev);

static Context *do_event(Event e, Context *c) {
  switch (e.event) {
  case EVENT_YIELD:
    // printf("do_event: EVENT_YIELD\n");
    c = schedule(c);
    break;
  case EVENT_SYSCALL:
    do_syscall(c);
    break;
  case EVENT_IRQ_TIMER:
    // PA4.1 由于native的AM在创建上下文的时候默认会打开中断, 为了成功运行native创建的内核线程, 你还需要在事件处理回调函数中识别出时钟中断事件.
    // 我们会在PA4的最后介绍时钟中断相关的内容, 目前识别出时钟中断事件之后什么都不用做, 直接返回相应的上下文结构即可.
    c = schedule(c);
    break;
  default:
    panic("Unhandled event ID = %d", e.event);
  }

  return c;
}

void init_irq(void) {
  Log("Initializing interrupt/exception handler...");
  cte_init(do_event);
}
