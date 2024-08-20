# 环境

没必要用vim。

关于IntelliSense，VSCode需要安装`c/c++`、`Makefile Tools`插件，至少这在PA1中还有点用。到PA2的abstract machine中，这个插件貌似就没法识别出`gcc -D`定义的一堆define了❓

> 因为是用make进行编译的复杂项目，`c/c++` IntelliSense 需要知道具体是怎么make的，所以必须安装`Makefile Tools`才不会出现一堆"Identifier not found"红线，见[Configure C/C++ IntelliSense - Configuration providers](https://code.visualstudio.com/docs/cpp/configure-intellisense#_configuration-providers)：`Ctrl+Shift+P` `C/C++ Change Configuration provider` -> Select `Makefile Tools`，"If it identifies only one custom configuration provider, this configuration provider is automatically configured for IntelliSense"

关于gdb调试，可以配置到VSCode里调试，传入`-nb`打印出make gdb时产出的gdb命令，配置vscode的`launch.json`。其实用惯了gdb，也就`b func` `n` `s` `layout split` `p EXPR`，直接在Terminal里调试也挺方便的。

> 以nemu部分的调试为例，`make gdb`给出了`gdb -s /home/hc/ics-pa-2023/nemu/build/riscv32-nemu-interpreter --args /home/hc/ics-pa-2023/nemu/build/riscv32-nemu-interpreter --log=/home/hc/ics-pa-2023/nemu/build/nemu-log.txt`，于是在vscode中生成调试配置文件`launch.json`，Add Configuation，选择`Nemu GDB`
> 
> ```json
> {
>     "version": "0.2.0",
>     "configurations": [
>         {
>             "name": "Nemu GDB",
>             "type": "cppdbg",
>             "preLaunchTask": "Make",
>             "request": "launch",
>             "program": "${workspaceFolder}/build/riscv32-nemu-interpreter",
>             "args": [
>                 "--log=${workspaceFolder}/build/nemu-log.txt",
>             ],
>             "stopAtEntry": false,
>             "cwd": "${fileDirname}",
>             "environment": [],
>             "externalConsole": false,
>             "MIMode": "gdb",
>             "setupCommands": [
>                 {
>                     "description": "Enable pretty-printing for gdb",
>                     "text": "-enable-pretty-printing",
>                     "ignoreFailures": true
>                 },
>                 {
>                     "description": "Set Disassembly Flavor to Intel",
>                     "text": "-gdb-set disassembly-flavor intel",
>                     "ignoreFailures": true
>                 }
>             ]
>         },
>     ]
> }
> ```

## （需要补习的）前置知识

- RISC-V：学过CS61C就够用了
- Makefile：虽然PA1中可以不用理解Makefile，但PA2里就需要全部读懂Makefile了，所以最好一开始就掌握make的语法。笔记见[Notes](https://github.com/Holence/Notes/blob/main/Tools/Make/Make.md)
- ELF：PA2.4和PA3.3中都要手写解析ELF，可以看看《System V generic ABI》第四五章，笔记见[Notes](https://github.com/Holence/Notes/blob/main/OS/ELF.md)

# PA1

制作简易的调试器（因为硬件都是模拟出来的，打印寄存器、内存也就是打印出数组中的值）。读代码，找到需要调用的函数或需要访问的static变量（应该是需要手动添加include的）。

```
(nemu) x 8 $pc
0x80000000: 0x00000297 0x00028823 0x0102c503 0x00100073 
0x80000010: 0xdeadbeef 0x5a5a5a5a 0x5a5a5a5a 0x5a5a5a5a 
(nemu) px *($pc)
0x00000297
(nemu) px *($pc+4*4)
0xdeadbeef
```

## 1.3

大致就是通过`make menuconfig`运行kconfig工具在terminal的图形化界面中进行个性化配置，产生的`include/config/auto.conf`将会用于`make`的个性化编译，产生的`include/generated/autoconf.h`将会用于c语言代码中的`#ifdef`。

对于那些看不懂的宏定义，可以在makefile中加入一行，让输出预编译的代码（出自[第5课的PPT](http://why.ink:8080/static/slides/ICS2023/05.pdf)）

```makefile
# ./nemu/scripts/build.mk
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	
# gcc preprocessing file, 方便理解那些看不懂的宏展开
	@$(CC) $(CFLAGS) -E -MF /dev/null $< | grep -ve '^#' | clang-format - > $(basename $@).i
	
	$(call call_fixdep, $(@:.o=.d), $@)
```

## 1.4

`p EXPR`指令需要自己写表达式求值的工具。

```
(nemu) p <expr>
其中 <expr> 为 <decimal-number>
  | <hexadecimal-number>    # 以"0x"开头
  | <reg_name>              # "$ra" / "$pc"
  | "(" <expr> ")"
  | <expr> "+" <expr>
  | <expr> "-" <expr>
  | <expr> "*" <expr>
  | <expr> "/" <expr>
  | <expr> "==" <expr>
  | <expr> "!=" <expr>
  | <expr> "&&" <expr>
  | "*" <expr>              # 指针解引用（只用支持数值或寄存器的值作为地址，不用支持gdb那样的变量作为地址）
```
 
最后随机生成测试，是先随机出一个表达式（与寄存器、内存相关的不用随机测试，自己手动测几个就行了），然后写入一个`temp.c`的临时文件，编译，开进程运行，`./gen-expr 1000 > input`生成多组结果和表达式

> 过滤除以0的case，可以用signal，但遇到0乘以时`(1 / 0) * 0`，运行却没有异常，这种实在没法探测
>
> 完全可能是很复杂的形态`(6-2*3)*(1 - 2/(1-1))`，到nemu里运行是先计算出`1-1`为0，再`2/0`，肯定会报错的。这种只能运行报错后手动删掉这种case了

最后修改nemu的main函数，读文件，调用`expr()`，对比结果

```c
// nemu-main.c (only for test expr)
#include <common.h>

void init_monitor(int, char *[]);
void am_init_monitor();
void engine_start();
int is_exit_status_bad();
word_t expr(char *e, bool *success);

int main(int argc, char *argv[]) {
  /* Initialize the monitor. */
#ifdef CONFIG_TARGET_AM
  am_init_monitor();
#else
  init_monitor(argc, argv);
#endif

  /* Start engine. */
  // engine_start();

  FILE *file = fopen("/home/hc/ics-pa-2023/nemu/tools/gen-expr/input", "r");
  if (file == NULL) {
    perror("Error opening file");
    return 1;
  }

  word_t reference;
  char expression[32];

  int total = 0;
  int correct = 0;
  bool success = true;
  while (fscanf(file, "%u %[^\n]", &reference, expression) == 2) {
    printf(ANSI_FMT("%s = %u\n", ANSI_FG_BLACK), expression, reference);
    total++;
    word_t result = expr(expression, &success);
    if (result != reference) {
      printf(ANSI_FMT("%u - Not Match\n", ANSI_FG_RED), result);
      getc(stdin);
    } else {
      correct++;
      printf(ANSI_FMT("Match\n", ANSI_FG_GREEN));
    }
  }
  fclose(file);

  printf(ANSI_FMT("Total Test: %d\nCorrect: %d\n", ANSI_FG_MAGENTA), total, correct);

  return is_exit_status_bad();
}
```

## 1.5

固定32个可用的watchpoint，用链表分别存储“使用中”、“空闲”队列。

如果`wp_check_changed()`中就只是`nemu_state.state = NEMU_STOP`，那运行时设置`w $pc`，然后一直`c`运行到最后，会到`hostcall.c: invalid_inst()`，说明是已经到了最后，却没退出成功。退出的时候是`ebreak`指令去做`set_nemu_state(NEMU_END, thispc, code)`，这时候就不需要再`wp_check_changed()`了。所以在`wp_check_changed()`中设置`nemu_state.state = NEMU_STOP`时，弄个判断：

```c
if (nemu_state.state != NEMU_END) {
  nemu_state.state = NEMU_STOP;
}
```

# PA2

## 2.2

编写RISC-V32I_M的模拟器，在外部用risc-v编译器，编译一些c语言写的rics-v机器码用于测试，之后用nemu运行之。

> [!IMPORTANT]
> - 指令集见《The RISC-V Instruction Set Manual Volume I Unprivileged Architecture》 Instruction Set Listings，其实UCB的green card也都够用了
> - imm解析可以参考[CS61CPU](https://cs61c.org/su24/projects/proj3/#task-7-2-immediate-generator)，要记得imm都是要sign-extend成32/64位的。
> - load进来的数据也要sign-extend
> - 做`mul`、`mulh`时需要把`uint32_t`转换为`int64_t`去做乘法。但由于bit extend的特性，从`uint32_t`到更多位的`int64_t`需要先到`int32_t`再到`int64_t`，这样才能让32位的负数正确地sign-extend扩展为64位的负数。

❓ecall要做吗？

为了方便测试，在`/abstract-machine/scripts/platform/nemu.mk`中的NEMUFLAGS加上`-b`，让传入nemu的参数开启batch mode，这样就不用每次开始运行了还要手动`c`运行和`q`退出。之后直接运行`make ARCH=riscv32-nemu run`就能运行所有的测试了。

> [!IMPORTANT]
> 应该在`/abstract-machine/Makefile`里把`CFLAGS   += -O2`改为`O0`，在`O2`优化的情况下，发现很多测试编译出来给check函数的`a0`直接就设为了编译器预想的值，根本没有运行nemu计算的指令！！❓至少在做cpu-test时是这样

- string和hello-str还需要实现额外的内容才能运行，现在运行会报错的，记得跳过（我就忘了，看到汇编里`sb a0,1016(a5) # a00003f8 <_end+0x1fff73f8>`写着超出了_end的地址，意识到不应该是我的问题，才到文档里查到需要跳过这两个测试）

❓很奇怪，当我在nemu中`make menuconfig`选中了Enable Address Sanitizer后，有时候编译就会报`AddressSanitizer:DEADLYSIGNAL`的错。

## Makefile解析

运行am-kernels中测试的时候，`make ARCH=riscv32-nemu ALL=dummy run`会生成`Makefile.dummy`，其中的内容为

```
NAME = dummy
SRCS = tests/dummy.c
include /abstract-machine/Makefile
```

接下来会`make -s -f Makefile.dummy ARCH=$(ARCH) $(MAKECMDGOALS)`去运行这个Makefile，其实就是在引用`/abstract-machine/Makefile`中的内容（PA2.3中会要求仔细阅读），其中先编译生成一堆OBJS，再`@$(LD) $(LDFLAGS) -o $(IMAGE).elf --start-group $(LINKAGE) --end-group`链接成ELF文件，最后用`@$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin`抽取出部分内容成为裸二进制文件，这是nemu需要的程序文件IMAGE（.bin），最后通过`$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin`运行nemu中Makefile的`make run`。

```makefile
image: $(IMAGE).elf
	@$(OBJDUMP) -d $(IMAGE).elf > $(IMAGE).txt
	@echo + OBJCOPY "->" $(IMAGE_REL).bin

# -S (--strip-all)
#    Do not copy relocation and symbol information from the source file.  Also deletes debug sections.
# -S之后只是少了些附加信息，依旧可以被linux运行、被readelf、被objdump
# -O binary 是保留raw binary file，不能被linux运行、被readelf、被objdump
    @$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(IMAGE).elf $(IMAGE).bin
```

## 2.3

实现几个与ISA无关的通用库函数，理解abstract machine作为nemu(cpu)与OS之间的中间层的奥义。

```
// trm.c
void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}
```

❓nemu相当于是一个可以一次性执行完一段程序最后返回一个return值的cpu，仅仅是一个图灵机，运行完就结束了。而我们想要的是更强大的能与外界交互的机器，运行完还能根据外界的反馈再次运行❓那么am就需要根据nemu返回的值来做相应的处理，这是在模拟中断？

❓用riscv64-linux-gnu-gcc的输出也能说明这点

```bash
cd /am-kernels/tests/cpu-tests
# 仅仅编译单个文件到汇编
riscv64-linux-gnu-gcc tests/dummy.c -S dummy.s # 发现里面没有ebreak
# 编译整个可执行文件
riscv64-linux-gnu-gcc tests/dummy.c -o dummy
riscv64-linux-gnu-objdump -d dummy > dump.txt # 发现在_start()函数中有ebreak，这并不是main()函数
```

## 2.4

itrace、iringbuf、mtrace就在nemu里动动手脚即可。

ftrace需要读取elf，要找出当前运行的指令行对应在哪个函数内，要找的是Section里的symtab和strtab两个表，symtab中有函数的地址信息，strtab里有所有字符串的信息。所以需要定位到Section Header Table，找到其中symtab和strtab两个表的地址。运行程序时碰到`jalr`和`jal`两个指令，判断是call还是ret，看当前指令的pc位于symtab中哪个function的地址范围内，则查找到strtab中该函数的名字。

要ftrace的话，运行nemu就需要额外传入elf文件（因为`$(IMAGE).bin`文件是raw binary，啥都没了），在`/abstract-machine/scripts/platform/nemu.mk`中设置运行nemu时要传入的参数`ARGS="-e $(IMAGE).elf $(NEMUFLAGS)"`

ftrace实现出来只是在实时打印全部的函数调用过程，用个int depth记录深度然后打印缩进即可，感觉也没多少机会会用这个功能。若要实现backtrace打印某个时刻的函数调用栈，得用栈的数据结构push、pop记录函数，懒得做了。

---

> [!NOTE]
> 不匹配的函数调用和返回，尝试结合反汇编结果, 分析为什么会出现这一现象：看反汇编代码，在f2中调用f1的是正常的`jalr rs`（`jalr ra, rs, 0`），所以触发打印log`call f1`，而f1跳向f0用的是`jr`（`jalr x0, rs, 0`），也就是不把pc存到`ra`就跳出去，将来不用跳回到这条指令+4的地方，而是直接跳回到f1被调用的地方，也就是f2中调用处+4的地方。`jr`指令因为没有存`ra`，也就没被monitor视为是在call，所以从f1跳入f0的时候并不会触发打印`call f0`，而f0要返回了，ret指令会触发打印`ret f0`，所以就出现了`call f1`接着`ret f0`的现象。
>
> 同理，为什么`call f1`对应的是`ret f3`？是因为f0中也是`jr`，`call f1`后隐藏地`call f0`，又隐藏地`call f3`，f3里正常调用f2两次，出来的时候自然打印了`ret f3`。
>
> 这里如果把`jr`这种情况也算作call的话，并不能解决问题，因为`jr`没有对应的`ret`，所以会匹配不上的。
> 
> ```
>   Call f2
>     Call f1 # then call f0, call f3
>       Call f2
>         Call f1 # then call f0
>         Ret  f0
>       Ret  f2
>       Call f2
>         Call f1 # then call f0
>         Ret  f0
>       Ret  f2
>     Ret  f3
>   Ret  f2
> ```

---

要编写详尽的test来测试klib，懒得自己写了，看有人引用了glibc的测试，我也引用一下吧: https://github.com/alelievr/libft-unit-test/blob/master/hardcore-mode/

> [!IMPORTANT]
> 先在native上用glibc的库函数来测试（先保证这些test本身书写正确）, 然后在native上测试你的klib测试（再保证klib正确）, 最后再到NEMU上运行这些测试代码来测试你的NEMU实现（最后保证nemu正确）

在native上测试klib时出现问题，只能用二分法找到出错的用例❓klib的部分没法调试啊？因为也还没做printf，就只能把对应的klib函数和测试用例复制到一个临时c中调试、修改（最好把函数名修改掉，如果就是什么strcmp，它也不报错，直接神不知鬼不觉地就去用c的库了？）。

---

difftest部分，`/nemu/src/cpu/difftest/ref.c`没有任何用处，在`nemu/src/cpu/difftest/dut.c`的`init_difftest()`中已经用`dlsym()`去`/nemu/tools/spike-diff/build/riscv32-spike-so`去寻找函数了，其实函数在`/nemu/tools/spike-diff/difftest.cc`中。

寻找spike中定义的寄存器顺序，在`/nemu/tools/spike-diff/repo/disasm/regnames.cc`中有，发现和nemu是一致的。

## 2.5

还不懂putch是怎么实现的

> [!NOTE]
> 理解mainargs，请你通过RTFSC理解这个参数是如何从make命令中传递到hello程序中的, `$ISA-nemu`和`native`采用了不同的传递方法：
>
> 这里`make ARCH=$ISA-nemu mainargs=I-love-PA run`
>
> `$ISA-nemu`：通过Makefile把`mainargs`编译到客户程序的IMAGE中：nemu.mk中`-DMAINARGS=\"$(mainargs)\"`，在`/am/src/platform/nemu/trm.c`中把`mainargs`存在`char mainargs[]`中，再到调用`hello.c`里的`int main(const char *args)`时，就传入了。
>
> `native`：通过`getenv()`获取到输入的`mainargs`，通过`static void init_platform() __attribute__((constructor))`，在`hello.c`的`main()`运行之前，做了很多其他的操作
>
> 一个示例
> 
> ```c
> #include <stdio.h>
> #include <stdlib.h>
> 
> int main(char *name);
> 
> void __attribute__((constructor)) before_main() {
>   printf("I can get your name from env.\n");
>   char *name = getenv("NAME");
>   int ret = main(name ? name : "root");
>   printf("After main()\n");
>   exit(ret);
>   printf("Nothing goes here\n");
> }
> 
> int main(char *name) {
>   printf("Greetings! @%s is in main()\n", name);
>   return 0;
> }
> 
> void __attribute__((destructor)) before_exit() {
>   printf("Before exiting\n");
> }
> ```
>
> 运行的时候`NAME=Lord ./FILE`就会输出：
>
> ```
> I can get your name from env.
> Greetings! @Holence is in main()
> After main()
> Before exiting
> ```

# 二周目问题

- 1.2 如果没有寄存器, 计算机还可以工作吗? 如果可以, 这会对硬件提供的编程模型有什么影响呢?
  就算你是二周目来思考这个问题, 你也有可能是第一次听到"编程模型"这个概念. 不过如果一周目的时候你已经仔细地阅读过ISA手册, 你会记得确实有这么个概念. 所以, 如果想知道什么是编程模型, RTFM吧.
- 1.3 对于GNU/Linux上的一个程序, 怎么样才算开始? 怎么样才算是结束? 对于在NEMU中运行的程序, 问题的答案又是什么呢?
- 1.4 我们在表达式求值中约定, 所有运算都是无符号运算. 你知道为什么要这样约定吗? 如果进行有符号运算, 有可能会发生什么问题?
- 1.6 如果你在运行稍大一些的程序(如microbench)的时候使用断点, 你会发现设置断点之后会明显地降低NEMU执行程序的效率. 思考一下这是为什么? 有什么方法解决这个问题吗?
与此相关的问题还有: NEMU中为什么要有nemu_trap? 为什么要有monitor?
- 1.6 [How debuggers work](https://eli.thegreenplace.net/2011/01/23/how-debuggers-work-part-1/)
- 2.3 为什么要有AM？操作系统也有自己的运行时环境. AM和操作系统提供的运行时环境有什么不同呢? 为什么会有这些不同?

TODO:
- nemu从0开始运行的每一步干了啥在