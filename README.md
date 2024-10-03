# ICS2023 Programming Assignment

This project is the programming assignment of the class ICS(Introduction to Computer System)
in Department of Computer Science and Technology, Nanjing University.

For the guide of this programming assignment,
refer to https://nju-projectn.github.io/ics-pa-gitbook/ics2023/

To initialize, run
```bash
bash init.sh subproject-name
```
See `init.sh` for more details.

The following subprojects/components are included. Some of them are not fully implemented.
* [NEMU](https://github.com/NJU-ProjectN/nemu)
* [Abstract-Machine](https://github.com/NJU-ProjectN/abstract-machine)
* [Nanos-lite](https://github.com/NJU-ProjectN/nanos-lite)
* [Navy-apps](https://github.com/NJU-ProjectN/navy-apps)

## How to Run

you need to use ssh to clone sub-repo from github. on a new machine, [generate a ssh key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent#generating-a-new-ssh-key). then [add ssh key to github](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/adding-a-new-ssh-key-to-your-github-account#adding-a-new-ssh-key-to-your-account)

```bash
bash init.sh nemu # set NEMU_HOME
bash init.sh abstract-machine # clone fceux-am and set AM_HOME
git submodule update --init # clone my am-kernels repo
bash init.sh navy-apps # set NAVY_HOME

sudo apt-get install build-essential    # build-essential packages, include binary utilities, gcc, make, and so on
sudo apt-get install man                # on-line reference manual
sudo apt-get install gcc-doc            # on-line reference manual for gcc
sudo apt-get install gdb                # GNU debugger
sudo apt-get install git                # revision control system
sudo apt-get install libreadline-dev    # a library used later
sudo apt-get install libsdl2-dev        # a library used later
sudo apt-get install llvm llvm-dev      # llvm project, which contains libraries used later

# menuconfig
cd nemu
sudo apt-get install clang-format bison flex
make menuconfig
# 关闭trace，打开device

# riscv交叉编译环境
sudo apt-get install g++-riscv64-linux-gnu binutils-riscv64-linux-gnu
```

[修复riscv32编译错误](https://nju-projectn.github.io/ics-pa-gitbook/ics2023/2.2.html) `fatal error: gnu/stubs-ilp32.h: No such file or directory`

```diff
--- /usr/riscv64-linux-gnu/include/gnu/stubs.h
+++ /usr/riscv64-linux-gnu/include/gnu/stubs.h
@@ -5,5 +5,5 @@
 #include <bits/wordsize.h>

 #if __WORDSIZE == 32 && defined __riscv_float_abi_soft
-# include <gnu/stubs-ilp32.h>
+//# include <gnu/stubs-ilp32.h>
 #endif
```

```bash
cd nanos-lite

# 打包navy用户程序到ramdisk.img
# 各种程序所需的数据文件具体参考PA3.5 https://nju-projectn.github.io/ics-pa-gitbook/ics2023/3.5.html
# fceux-am所需的rom文件: https://box.nju.edu.cn/f/3e56938d9d8140a7bb75/?dl=1
# pal的数据文件，见note.md
make ARCH=riscv32-nemu update
# 运行nanos-lite
make ARCH=riscv32-nemu run_batch
```
