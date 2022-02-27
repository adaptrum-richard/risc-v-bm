#include <printk.h>
extern char load_isa_la_lb(int index);
void asm_test(void)
{
    printk("run %s\n", __func__);
    for(int index; index < 14; index++)
        printk("%c", load_isa_la_lb(index));
    printk("end %s\n", __func__);
}
void start()
{
    asm_test();
    printk("start\n");
}