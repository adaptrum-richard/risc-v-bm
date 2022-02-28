#include <printk.h>
extern char load_isa_la_lb(int index);
unsigned long load_isa_ld(unsigned long *array, int index);
unsigned int load_isa_lh(unsigned int *array, int index);
unsigned int load_isa_lhu(unsigned int *array, int index);
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
    printk("end %s\n", __func__);
}
void start()
{
    asm_test();
    printk("start\n");
}