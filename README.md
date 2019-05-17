Linux 0.11
======

Linux 0.11 相关学习笔记都注释在代码里了<br>

Mac 或 Ubuntu 下编译 Linux 0.11 <br>
1) 直接下载工程，进入 linux-0.11-lab 目录，终端执行 make clean;make; <br>
2) 终端执行 make start-hd; <br>

Mac 或 Ubuntu 下用 qemu 模拟 Linux 0.11 <br>
1) Mac 安装 qemu，终端执行 brew install qemu <br>
2) Ubuntu 安装 qemu，终端执行 sudo apt-get install qemu-kvmc <br>
3) 终端执行 make start-hd，会启动 qemu，不用管，直接 Ctrl + C 关闭 qemu <br>
4）终端执行 qemu-system-i386 -S -s -parallel stdio -m 512M -boot a -fda src/Image -hda rootfs/hdc-0.11.img <br>

Mac 或 Ubuntu 下用 gdb 调试 Linux 0.11 <br>
1) 终端下执行 gdb -q -tui -x tools/gdbinit <br>

