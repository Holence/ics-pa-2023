# 环境

没必要用vim。

VSCode安装`c/c++`、`Makefile Tools`插件

> 因为是用make进行编译的复杂项目，`c/c++` IntelliSense 需要知道具体是怎么make的，所以必须安装`Makefile Tools`才不会出现一堆"Identifier not found"红线，见[Configure C/C++ IntelliSense - Configuration providers](https://code.visualstudio.com/docs/cpp/configure-intellisense#_configuration-providers)：`Ctrl+Shift+P` `C/C++ Change Configuration provider` -> Select `Makefile Tools`，"If it identifies only one custom configuration provider, this configuration provider is automatically configured for IntelliSense"

gdb调试，nemu中`make gdb`给出了`gdb -s /home/hc/ics-pa-2023/nemu/build/riscv32-nemu-interpreter --args /home/hc/ics-pa-2023/nemu/build/riscv32-nemu-interpreter --log=/home/hc/ics-pa-2023/nemu/build/nemu-log.txt`，于是在vscode中生成调试配置文件`launch.json`，Add Configuation，选择`(gdb) Launch`

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(gdb) Launch",
            "type": "cppdbg",
            "preLaunchTask": "Make",
            "request": "launch",
            "program": "${workspaceFolder}/build/riscv32-nemu-interpreter",
            "args": [
                "--log=${workspaceFolder}/build/nemu-log.txt",
            ],
            "stopAtEntry": false,
            "cwd": "${fileDirname}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                },
                {
                    "description": "Set Disassembly Flavor to Intel",
                    "text": "-gdb-set disassembly-flavor intel",
                    "ignoreFailures": true
                }
            ]
        },
    ]
}
```

## make

虽然PA1中可以不用理解Makefile，但PA2里就需要全部读懂Makefile了。所以最好一开始就掌握make的语法，教程：[Makefile Tutorial By Example](https://makefiletutorial.com)、[官方手册](https://www.gnu.org/software/make/manual/make.html)。

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

## RTFSC

大致就是通过`make menuconfig`运行kconfig工具在terminal的图形化界面中进行个性化配置，产生的`include/config/auto.conf`将会用于`make`的个性化编译，产生的`include/generated/autoconf.h`将会用于c语言代码中的`#ifdef`。

对于那些看不懂的宏定义，可以在makefile中加入一行，让输出预编译的代码（出自[第5课的PPT](http://why.ink:8080/static/slides/ICS2023/05.pdf)）

```make
# ./nemu/scripts/build.mk
$(OBJ_DIR)/%.o: %.c
	@echo + CC $<
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c -o $@ $<
	
	# gcc preprocessing file, 方便理解那些看不懂的宏展开
	@$(CC) $(CFLAGS) -E -MF /dev/null $< | grep -ve '^#' | clang-format - > $(basename $@).i
	
	$(call call_fixdep, $(@:.o=.d), $@)
```

## 表达式求值

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

## 监视点

固定32个可用的watchpoint，用链表分别存储“使用中”、“空闲”队列。

如果设置`w $pc`然后一直`c`运行到最后，会到`hostcall.c: invalid_inst()`？是因为在`wp_check_changed()`中设置`nemu_state.state = NEMU_STOP`吗？

# PA2

## 2.2 RTFM & RTFSC

编写RISC-V32I_M的模拟器，在外部用risc-v编译器，编译一些c语言写的rics-v机器码用于测试，之后用nemu运行之。

注意：
- 指令集见《The RISC-V Instruction Set Manual Volume I Unprivileged Architecture》 Instruction Set Listings，其实UCB的green card也都够用了
- imm解析可以参考[CS61CPU](https://cs61c.org/su24/projects/proj3/#task-7-2-immediate-generator)，要记得imm都是要sign-extend成32/64位的。
- load进来的数据也要sign-extend
- 做`mul`、`mulh`时需要把`uint32_t`转换为`int64_t`去做乘法。但由于bit extend的特性，从`uint32_t`到更多位的`int64_t`需要先到`int32_t`再到`int64_t`，这样才能让32位的负数正确地sign-extend扩展为64位的负数。

❓ecall要做吗？

运行am-kernels中测试的时候，`make ARCH=riscv32-nemu ALL=dummy run`会生成`Makefile.dummy`，其中的内容为

```
NAME = dummy
SRCS = tests/dummy.c
include /abstract-machine/Makefile
```

接下来会`make -s -f Makefile.dummy ARCH=$(ARCH) $(MAKECMDGOALS)`去运行这个Makefile，其实都是在引用`/abstract-machine/Makefile`中的内容（PA2.3中会要求仔细阅读），其中编译生成nemu需要的程序文件IMAGE（.bin），再通过`$(MAKE) -C $(NEMU_HOME) ISA=$(ISA) run ARGS="$(NEMUFLAGS)" IMG=$(IMAGE).bin`运行nemu中Makefile的`make run`。

为了方便测试，在`/abstract-machine/scripts/platform/nemu.mk`中的NEMUFLAGS加上`-b`，让传入nemu的参数开启batch mode，这样就不用每次开始运行了还要手动`c`运行和`q`退出。之后直接运行`make ARCH=riscv32-nemu run`就能运行所有的测试了。

- string和hello-str还需要实现额外的内容才能运行，现在运行会报错的，记得跳过（我就忘了，看到汇编里`sb a0,1016(a5) # a00003f8 <_end+0x1fff73f8>`写着超出了_end的地址，意识到不应该是我的问题，才到文档里查到需要跳过这两个测试）

❓很奇怪，当我在nemu中`make menuconfig`选中了Enable Address Sanitizer后，有时候编译就会报`AddressSanitizer:DEADLYSIGNA`的错。

# 2.3

```
// trm.c
void _trm_init() {
  int ret = main(mainargs);
  halt(ret);
}
```

❓nemu相当于是一个可以一次性执行完一段程序最后返回一个return值的cpu，仅仅是一个图灵机，运行完就结束了。而我们想要的是更强大的能与外界交互的机器，运行完还能根据外界的反馈再次运行❓那么am就需要根据nemu返回的值来做相应的处理，这是在模拟中断？

## 二周目问题

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