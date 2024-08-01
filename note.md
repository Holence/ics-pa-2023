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
            "request": "launch",
            "program": "/home/hc/ics-pa-2023/nemu/build/riscv32-nemu-interpreter",
            "args": [
                "--log=/home/hc/ics-pa-2023/nemu/build/nemu-log.txt"
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

# PA1

制作简易的调试器（因为硬件都是模拟出来的，打印寄存器、内存也就是打印出数组中的值）。读代码，找到需要调用的函数或需要访问的static变量（应该是需要手动添加include的）。

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
  | "*" <expr>              # 指针解引用（不用解引用程序中变量，只是把数值或寄存器中的值当作地址，再从内存中取出即可）
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

非常尴尬的是，我做test的用时 比 写表达式求值逻辑的用时 还长😅

# 二周目

- 1.2 如果没有寄存器, 计算机还可以工作吗? 如果可以, 这会对硬件提供的编程模型有什么影响呢?
  就算你是二周目来思考这个问题, 你也有可能是第一次听到"编程模型"这个概念. 不过如果一周目的时候你已经仔细地阅读过ISA手册, 你会记得确实有这么个概念. 所以, 如果想知道什么是编程模型, RTFM吧.

