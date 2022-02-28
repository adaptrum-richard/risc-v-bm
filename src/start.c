#include <printk.h>
extern char load_isa_la_lb(int index);
unsigned long load_isa_ld(unsigned long *array, int index);
unsigned int load_isa_lh(unsigned int *array, int index);
unsigned int load_isa_lhu(unsigned int *array, int index);
unsigned long store_isa_sd(unsigned long *addr, unsigned long v);
unsigned int store_isa_sw(unsigned int *addr, unsigned int v);
unsigned short store_isa_sh(unsigned short *addr, unsigned short v);
unsigned char store_isa_sb(unsigned char *addr, unsigned char v);
void asm_test(void)
{
    printk("run %s\n", __func__);
    printk("test lb la isa:\n");
    for(int index; index < 14; index++)
        printk("%c", load_isa_la_lb(index));
    
    unsigned long array[9] = {0xfffffffffffffff1, 0xfffffffffffffff2, 0xfffffffffffffff3, 
                            0xfffffffffffffff4, 0xfffffffffffffff5, 0xfffffffffffffff6, 
                            0xfffffffffffffff7 ,0xfffffffffffffff8, 0xfffffffffffffff9};
    printk("test ld isa:\n");
    for(int index = 0; index < 9; index++)
        printk("0x%lx\n", load_isa_ld(array, index));

    printk("test lh isa:\n");
    for(int index = 0; index < 9*2; index++)
        printk("0x%lx\n", load_isa_lh((unsigned int*)array, index));

    printk("test lhu isa:\n");
    for(int index = 0; index < 9*2; index++)
        printk("0x%lx\n", load_isa_lhu((unsigned int*)array, index));

    printk("test sd sw sh sb isa:\n");
    unsigned long v1 = 0;
    unsigned long v2 = 0x1111111111111111;
    printk("test sd: 0x%lx\n", store_isa_sd(&v1, v2));
    v1 = 0;
    printk("test sw: 0x%lx\n", store_isa_sw((unsigned int *)&v1, (unsigned int)v2));
    v1 = 0;
    printk("test sh: 0x%lx\n", store_isa_sh((unsigned short *)&v1, (unsigned short)v2));
    v1 = 0;
    printk("test sb: 0x%lx\n", store_isa_sb((unsigned char *)&v1, (unsigned char)v2));

    printk("end %s\n", __func__);
}
void start()
{
    asm_test();
    printk("start\n");
}