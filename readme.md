## 支持功能
* 中断处理
    * timer，外部中断
* 内存映射
* 虚拟内存管理 
    * brk，sbrk
* 进程调度
    * fork，exec，wait，exit，抢占调度
* 内存管理
    * 伙伴系统，简易slub
* 系统调用
* 简易文件系统
* 内核功能
    * 等待队列，定时器，spinlock，atomic
* 驱动
    * 串口驱动，e1000驱动
* shell
    * ls，echo，cat，mkdir，rm
* 网络
    * dhcpc，udp，arp，ip
* debug
    * 支持打印backtrace
* TODO
    * ICMP，DNS，TCP，httpserver，多核调度，CFS调度器
## 环境搭建
* riscv工具链安装
1. [riscv gcc工具链安装](https://blog.csdn.net/dai_xiangjun/article/details/123040325)
2. [gdb-multiarch工具安装](https://blog.csdn.net/dai_xiangjun/article/details/123073604)

目前只在ubuntu20.04和22.04上测试。

## 运行与调试
* 运行
1. 在一个终端中先编译,直接make
```
$ make
.......
mkfs/mkfs fs.img README user/_init user/initcode.out user/_sh user/_cat user/_ls user/_echo user/_mkdir user/_rm user/_pipetest user/_udptest  user/_dhcpc
nmeta 46 (boot, super, log blocks 30 inode blocks 13, bitmap blocks 1) blocks 954 total 1000
balloc: first 628 blocks have been allocated
balloc: write bitmap block at sector 45
```

2. 运行程序
```
$ make run
qemu-system-riscv64 -machine virt -m 128M  -bios none -kernel kernel_build/kernel.elf  -nographic -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -netdev user,id=net0,hostfwd=udp::26999-:2000,net=192.168.100.0/24,dhcpstart=192.168.100.100 -object filter-dump,id=net0,netdev=net0,file=packets.pcap -device e1000,netdev=net0,bus=pcie.0
boot
kernel image: 0x80000000 - 0x80033000, 208896 bytes
Memory: 130868KB available, 32717 free pages, kernel_size: 204KB
mem_map:0x80033000, size:2097152 (512 pages), offset:0
dump all memblock regions:
0x80000000 - 0x80233000, flags: RESERVE
0x80233000 - 0x88000000, flags: FREE
Memory: total 128MB, add 128820KB to buddy, 2252KB reserve
Max order:10, pageblock: 4MB
Free pages at order        0     1     2     3     4     5     6     7     8     9    10
Node  0, zone   Noraml     1     0     1     1     0     0     1     1     1     0    31
magic                   0x10203040
size                    1000 blocks
nblocks                 954
ninodes                 200
nlog                    30
logstart block          2
inodestart block        32
bmapstart block         45
user_beigin = 0x80310000, user_end = 0x80310078, pc = 0x0, process = 0x80310000
now, run fork
run parent thread
run cmd dhcpc:
mac: 52:54:00:12:34:56
net ip 192.168.100.100 netmask 255.255.255.0 gw 192.168.100.2 dns 192.168.100.3
$ 
```
执行ctrl+a+x可以退出qemu

* 调试
1. 启动一个终端执行
```
$ make debug
qemu-system-riscv64 -machine virt -m 128M  -bios none -nographic -kernel kernel_build/kernel.elf -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0 -netdev user,id=net0,hostfwd=udp::26999-:2000,net=192.168.100.0/24,dhcpstart=192.168.100.100 -object filter-dump,id=net0,netdev=net0,file=packets.pcap -device e1000,netdev=net0,bus=pcie.0 -S -s
```

2. 启动另外一个终端
```
$ make gdb
```
![](https://github.com/adaptrum-richard/risc-v-bm/raw/dev/pic/gdb.jpg) 
