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

# 二周目

- 1.2 如果没有寄存器, 计算机还可以工作吗? 如果可以, 这会对硬件提供的编程模型有什么影响呢?
  就算你是二周目来思考这个问题, 你也有可能是第一次听到"编程模型"这个概念. 不过如果一周目的时候你已经仔细地阅读过ISA手册, 你会记得确实有这么个概念. 所以, 如果想知道什么是编程模型, RTFM吧.

