#include "testcode.h"
#include "exec.h"
#include "page.h"
#include "types.h"
#include "exec.h"
#include "printk.h"
#include "param.h"
#include "fs.h"
#include "string.h"
#include "fcntl.h"
#include "sysfile.h"

/*
$ riscv64-linux-gnu-readelf -l initcode.out 

Elf file type is EXEC (Executable file)
Entry point 0x0
There is 1 program header, starting at offset 64

Program Headers:
  Type           Offset             VirtAddr           PhysAddr
                 FileSiz            MemSiz              Flags  Align
  LOAD           0x0000000000000078 0x0000000000000000 0x0000000000000000
                 0x0000000000000060 0x0000000000000060  RWE    0x8

 Section to Segment mapping:
  Segment Sections...
   00     .text .got 

偏移值0x78, 长度0x60
$ hexdump initcode.out 
0000000 457f 464c 0102 0001 0000 0000 0000 0000
0000010 0002 00f3 0001 0000 0000 0000 0000 0000
0000020 0040 0000 0000 0000 0328 0000 0000 0000
0000030 0004 0000 0040 0038 0001 0040 0006 0005
0000040 0001 0000 0007 0000 0078 0000 0000 0000
0000050 0000 0000 0000 0000 0000 0000 0000 0000
0000060 0060 0000 0000 0000 0060 0000 0000 0000
0000070 0008 0000 0000 0000 0517 0000 3503 0505
0000080 0597 0000 b583 0505 0893 0070 0073 0000
0000090 0893 0020 0073 0000 f0ef ff9f 692f 696e
00000a0 0074 2400 0000 0000 0000 0000 0000 0000
00000b0 ffff ffff ffff ffff 0000 0000 0000 0000
00000c0 0000 0000 0000 0000 0024 0000 0000 0000
......
*/
void test_read_initcode()
{
    uint64 pc = -1;
    uint64 size = -1;
    uint64 *code = (uint64 *)get_free_page();
    int ret = read_initcode(code, &size, &pc);
    if(ret < 0){
        printk("%s %d:failed\n", __func__, __LINE__);
        return;
    }
    char *str = (char *)get_free_pages(1);
    char *p = str;
    char *src = (char*)code;
    for(int i = 0; i < size; i++){
        p += sprintf(p, "%02x ", src[i]);
        if(((i+1) % 16) == 0)
            p += sprintf(p, "%s", "\n");
    }
    printk("pc:0x%llx, size = 0x%llx, "
        "program code:\n"
        "---------------------------------------------\n", pc, size);
    printk("%s", str);
    printk("---------------------------------------------\n");
    free_page((unsigned long)code);
    free_pages((unsigned long)str, 1);
}

void test_console()
{
    int fd, ret;
    char buf[8];
    if((fd = __sys_open("/console", O_RDWR)) < 0){
        __sys_mknod("/console", CONSOLE, 0);
        fd = __sys_open("/console", O_RDWR);
    }
    if(fd <  0){
        panic("test_console\n");
    }
    __sys_close(fd);
    int max = 100;
    while(1){
        fd = __sys_open("/console", O_RDWR);
        if(fd <  0){
            panic("test_console\n");
        }   
        memset(buf, 0x0, sizeof(buf));
        int i = 0;
        while(i < max){
            char c;
            ret = __sys_read(fd, &c, 1);
            if(ret < 1)
                break;
            buf[i++] = c;
            if(c == '\n' || c == '\r')
                break;
        }
        buf[i] = '\0';
        int j = 0;
        while(buf[j] != '\0'){
            __sys_write(fd, &buf[j], 1);
            j++;
        }
        __sys_close(fd);
    }
 
    __sys_close(fd);
}

void test_sysfile(uint64 arg)
{
    int ret;
    char path[MAXPATH] = {0};
    char buf[MAXPATH] = {0};

    sprintf(path, "/tmp/%s", (char*)arg);
    int fd = __sys_open("/test", O_CREATE| O_WRONLY);

    if(fd < 0){
        printk("%s:%d __sys_open %d failed\n", __func__, __LINE__, fd);
        panic("open failed\n");
    }
    for(int i = 0; i < 3; ++i){
        ret = __sys_write(fd, path, strlen(path));
        if(ret < 0){
            printk("%s:%d __sys_write failed\n", __func__, __LINE__);
            panic("failed");
        }
        printk("%s:%d: write  %s , len = %d\n", __func__, __LINE__, path, ret);
        //sleep(1);
    }
    __sys_close(fd);

    fd = __sys_open("/test", O_RDONLY);
    if(fd < 0){
        printk("%s:%d __sys_open %d failed\n", __func__, __LINE__, fd);
        panic("open failed\n");
    }
    
    ret = __sys_read(fd, buf, sizeof(buf));
    __sys_close(fd);
    printk("read %s, len = %d\n", buf, ret);
    test_console();
}