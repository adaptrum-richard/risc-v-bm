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

#define barrier() asm volatile("": : :"memory")
#define smp_store_mb(var, value)  do { WRITE_ONCE(var, value); __smp_mb(); } while (0)

#define smp_store_release(p, v) do { __smp_wmb(); WRITE_ONCE(*p, v);}while(0)

# define smp_load_acquire(p)			\
({						\
    typeof(*p) ___p1 = READ_ONCE(*p);	\
    __smp_rmb();				\
    ___p1;   \
})

#endif
