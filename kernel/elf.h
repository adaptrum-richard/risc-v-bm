#ifndef __ELF_H__
#define __ELF_H__
#include "types.h"
// Format of an ELF executable file

#define ELF_MAGIC 0x464C457FU // "\x7FELF" in little endian

// File header
struct elfhdr
{
    uint magic; // must equal ELF_MAGIC
    uchar elf[12];
    ushort type;
    ushort machine;
    uint version;
    uint64 entry;
    uint64 phoff;
    uint64 shoff;
    uint flags;
    ushort ehsize;
    ushort phentsize;
    ushort phnum;
    ushort shentsize;
    ushort shnum;
    ushort shstrndx;
};

// Program section header
struct proghdr
{
    uint32 type;
    uint32 flags;
    uint64 off;
    uint64 vaddr;
    uint64 paddr;
    uint64 filesz;
    uint64 memsz;
    uint64 align;
};

// Values for Proghdr type
#define ELF_PROG_LOAD 1

// Flag bits for Proghdr flags
#define ELF_PROG_FLAG_EXEC 1
#define ELF_PROG_FLAG_WRITE 2
#define ELF_PROG_FLAG_READ 4
/*
参考：https://www.cnblogs.com/jiqingwu/p/elf_format_research_01.html
http://www.skyfree.org/linux/references/ELF_Format.pdf
目标代码文件的内容是由section组成的，而可执行文件的内容是由segment组成的

常用命令，可用于调试
--------------------------------------------------------------------------
readelf -h查看目标文件的头信息
$ riscv64-linux-gnu-readelf -h _init
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  Data:                              2's complement, little endian
  Version:                           1 (current)
  OS/ABI:                            UNIX - System V
  ABI Version:                       0
  Type:                              EXEC (Executable file)
  Machine:                           RISC-V
  Version:                           0x1
  Entry point address:               0xaa6
  Start of program headers:          64 (bytes into file)
  Start of section headers:          23896 (bytes into file)
  Flags:                             0x5, RVC, double-float ABI
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         2
  Size of section headers:           64 (bytes)
  Number of section headers:         15
  Section header string table index: 14

--------------------------------------------------------------------------
readelf -S 查看各个section的信息

$ riscv64-linux-gnu-readelf -S _init
There are 15 section headers, starting at offset 0x5d58:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  000000b0
       0000000000000ab4  0000000000000000 WAX       0     0     2
  [ 2] .rodata           PROGBITS         0000000000000ab8  00000b68
       000000000000029c  0000000000000000   A       0     0     8
  [ 3] .comment          PROGBITS         0000000000000000  00000e04
       0000000000000029  0000000000000001  MS       0     0     1
  [ 4] .debug_aranges    PROGBITS         0000000000000000  00000e2d
       0000000000000090  0000000000000000           0     0     1
  [ 5] .debug_info       PROGBITS         0000000000000000  00000ebd
       0000000000000ce8  0000000000000000           0     0     1
  [ 6] .debug_abbrev     PROGBITS         0000000000000000  00001ba5
       000000000000032f  0000000000000000           0     0     1
  [ 7] .debug_line       PROGBITS         0000000000000000  00001ed4
       00000000000013f7  0000000000000000           0     0     1
  [ 8] .debug_frame      PROGBITS         0000000000000000  000032d0
       00000000000002b8  0000000000000000           0     0     8
  [ 9] .debug_str        PROGBITS         0000000000000000  00003588
       0000000000000226  0000000000000001  MS       0     0     1
  [10] .debug_loc        PROGBITS         0000000000000000  000037ae
       0000000000001e3c  0000000000000000           0     0     1
  [11] .debug_ranges     PROGBITS         0000000000000000  000055ea
       0000000000000040  0000000000000000           0     0     1
  [12] .symtab           SYMTAB           0000000000000000  00005630
       0000000000000540  0000000000000018          13    19     8
  [13] .strtab           STRTAB           0000000000000000  00005b70
       0000000000000149  0000000000000000           0     0     1
  [14] .shstrtab         STRTAB           0000000000000000  00005cb9
       0000000000000098  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  p (processor specific)


--------------------------------------------------------------------------
readelf -l可以显示文件的program header信息
只有类型为LOAD的段是运行时真正需要的
$ riscv64-linux-gnu-readelf -l _init

Elf file type is EXEC (Executable file)
Entry point 0xaa6
There are 2 program headers, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x00000000000000b0 0x0000000000000000 0x0000000000000000
                 0x0000000000000d54 0x0000000000000d54  RWE    0x8
  GNU_STACK      0x0000000000000000 0x0000000000000000 0x0000000000000000
                 0x0000000000000000 0x0000000000000000  RW     0x10

 Section to Segment mapping:
  Segment Sections...
   00     .text .rodata 
   01     
--------------------------------------------------------------------------
以上信息可以用一条命令全部输出
$ riscv64-linux-gnu-readelf -a _init
*/
#endif
