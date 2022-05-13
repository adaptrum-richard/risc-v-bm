#ifndef __BARRIER_H__
#define __BARRIER_H__

#define READ_ONCE(x) (*(const volatile typeof(x) *)&(x)) 

#define WRITE_ONCE(x, val) \
do {                \
    *(volatile typeof(x) *)&(x) = (val); \
}while(0)

#define RISCV_FENCE(p, s) \
    __asm__ volatile ("fence " #p "," #s ::: "memory")

/*设备和内存间的屏障*/
#define mb()    RISCV_FENCE(iorw, iorw)
#define rmb()   RISCV_FENCE(ir, ir)
#define wmb()   RISCV_FENCE(ow, ow)

/*内存与内存间的屏障*/
#define __smp_mb()    RISCV_FENCE(rw, rw)
#define __smp_rmb()   RISCV_FENCE(r, r)
#define __smp_wmb()   RISCV_FENCE(w, w)

#endif
