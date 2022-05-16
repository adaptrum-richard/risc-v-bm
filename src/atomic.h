#ifndef __ATOMIC_H__
#define __ATOMIC_H__
#include "barrier.h"
#include "cmpxchg.h"

typedef struct {
    int counter;
}atomic_t;

/*
riscv AMO指令
amoadd.X X表示w或d，w表示4字节,d表示8字节
amoadd.X rd, rs2,(rs1)
将内存中地址为 x[rs1]中的双字记为 t，把这个值变为 t+x[rs2]，把x[rd]设为 t。
*/

#define ATOMIC_OP(op, asm_op, I, asm_type, c_type) \
static inline void ____atomic_##op(c_type i, atomic_t *v) \
{   \
    __asm__ volatile(   \
        "amo"#asm_op"." #asm_type " zero, %1, %0" \
        : "+A" (v->counter) \
        : "r" (I) \
        : "memory" \
    ); \
}

#define ATOMIC_OPS(op, asm_op, I) \
    ATOMIC_OP(op, asm_op, I, w, int)

ATOMIC_OPS(add, add,  i)
ATOMIC_OPS(sub, add, -i)
ATOMIC_OPS( or,  or,  i)
ATOMIC_OPS(xor, xor,  i)


#define atomic_add  ____atomic_add
#define atomic_sub  ____atomic_sub
#define atomic_or   ____atomic_or
#define atomic_xor  ____atomic_xor

#undef ATOMIC_OP
#undef ATOMIC_OPS

static inline int ____atomic_read(const atomic_t *v)
{
    return READ_ONCE(v->counter);
}

static inline void ____atomic_set(atomic_t *v, int i)
{
    WRITE_ONCE(v->counter, i);
}

#define atmoic_read __atomic_read
#define atomic_set  __atomic_set

#define ATOMIC_FETCH_OP(op, asm_op, I, asm_type, c_type) \
static inline c_type ____atomic_fetch_##op##_relaxed(c_type i, \
            atomic_t *v)    \
{   \
    register c_type ret;    \
    __asm__ volatile (      \
        "amo" #asm_op "." #asm_type " %1, %2, %0" \
        :"+A" (v->counter), "=r"(ret) \
        :"r"(I) \
        :"memory" \
    ); \
    return ret; \
}   \
static inline c_type ____atomic_fetch_##op(c_type i, atomic_t *v) \
{   \
    register c_type ret;    \
    __asm__ volatile (      \
        "amo" #asm_op "." #asm_type ".aqrl %1, %2, %0" \
        :"+A" (v->counter), "=r"(ret) \
        :"r"(I) \
        :"memory" \
    ); \
    return ret; \
}

#define ATOMIC_OP_RETURN(op, asm_op, c_op, I, asm_type, c_type) \
static inline c_type ____atomic_##op##_return_relaxed(c_type i, atomic_t *v) \
{   \
    return ____atomic_fetch_##op##_relaxed(i, v) c_op I; \
}   \
static inline c_type ____atomic_##op##_return(c_type i, atomic_t *v) \
{   \
    return ____atomic_fetch_##op(i, v) c_op I; \
}   

#define ATOMIC_OPS(op, asm_op, c_op, I) \
    ATOMIC_FETCH_OP(op, asm_op,        I, w, int) \
    ATOMIC_OP_RETURN(op, asm_op, c_op, I, w, int) 

ATOMIC_OPS(add, add, + ,i)
ATOMIC_OPS(sub, add, +, -i)

/*relaxed表示不会有任何内存屏障保护，未带relaxed表示有aq和rl*/

/*返回旧值*/
#define atomic_fetch_add_relaxed    ____atomic_fetch_add_relaxed
#define atomic_fetch_add            ____atomic_fetch_add
#define atomic_fetch_sub_relaxed    ____atomic_fetch_sub_relaxed
#define atomic_fetch_sub            ____atomic_fetch_sub

/*返回新值*/
#define atomic_add_return_relaxed   ____atomic_add_return_relaxed
#define atomic_add_return           ____atomic_add_return
#define atomic_sub_return_relaxed   ____atomic_sub_return_relaxed
#define atomic_sub_return           ____atomic_sub_return

#undef ATOMIC_OPS

/*and or xor*/
#define ATOMIC_OPS(op, asm_op, I)   \
    ATOMIC_FETCH_OP(op, asm_op, I, w, int)

ATOMIC_OPS(and, and, i)
ATOMIC_OPS( or,  or, i)
ATOMIC_OPS(xor, xor, i)

/*返回旧值*/
#define atomic_fetch_and_relaxed    ____atomic_fetch_and_relaxed
#define atomic_fetch_and            ____atomic_fetch_and
#define atomic_fetch_or_relaxed     ____atomic_fetch_or_relaxed
#define atomic_fetch_or             ____atomic_fetch_or
#define atomic_fetch_xor_relaxed    ____atomic_fetch_xor_relaxed
#define atomic_fetch_xor            ____atomic_fetch_xor

#undef ATOMIC_OPS
#undef ATOMIC_FETCH_OP
#undef ATOMIC_OP_RETURN

/*
atomic_xchg_acquire:
将i值存储在v->couter中，返回旧值。不带任何内存屏障
*/
static inline int atomic_xchg_relaxed(atomic_t *v, int i)
{
    return arch_xchg_relaxed(&(v->counter), i);
}

/*
atomic_xchg_acquire:
将i值存储在v->counter中,返回旧值。带加载获取内存屏障
v->counter = i
-------------
    ^
    |___ read
*/
static inline int atomic_xchg_acquire(atomic_t *v, int i)
{
    return arch_xchg_acquire(&(v->counter), i);
}

/*
atomic_xchg_release:
将i值存储在v->counter中,返回旧值。带存储释放内存屏障
    .-------wirte
    |
    V
-------------
v->counter = i
-------------
*/
static inline int atomic_xchg_release(atomic_t *v, int i)
{
    return arch_xchg_release(&(v->counter), i);
}

/*
atomic_xchg:
将i值存储在v->counter中,返回旧值。带加载获取-存储释放内存屏障
    .-------write
    |
    V
-------------
v->counter = i
-------------
    ^
    |___ read
*/
static inline int atomic_xchg(atomic_t *v, int i)
{
    return arch_xchg(&(v->counter), i);
}

/*
arch_cmpxchg_relaxed:
1.将val与v->counter比较，val与v->counter相等，则v->counter = new,
2.返回v->counter的原始值。
3.此函数不带任何内存屏障
*/
static inline int atomic_cmpxchg_relaxed(atomic_t *v, int val, int new)
{
    return arch_cmpxchg_relaxed(&(v->counter), val, new);
}

/*
atomic_cmpxchg_acquire:
1.将val与v->counter比较，val与v->counter相等，则v->counter = new,
2.返回v->counter的原始值。
3.加载获取内存屏障
ret = v->counter ;
v->counter = (val == v->counter) ? new: v->counter;
-------------
    ^
    |___ read
*/
static inline int atomic_cmpxchg_acquire(atomic_t *v, int val, int new)
{
    return arch_cmpxchg_acquire(&(v->counter), val, new);
}

/*
atomic_cmpxchg_release:
1.将val与v->counter比较，val与v->counter相等，则v->counter = new,
2.返回v->counter的原始值。
3.加载获取内存屏障
    .-------write
    |
    V
----------------
ret = v->counter ;
v->counter = (val == v->counter) ? new: v->counter;
*/
static inline int atomic_cmpxchg_release(atomic_t *v, int val, int new)
{
    return arch_cmpxchg_release(&(v->counter), val, new);
}

/*
atomic_cmpxchg_release:
1.将val与v->counter比较，val与v->counter相等，则v->counter = new,
2.返回v->counter的原始值。
3.加载获取内存屏障
    .-------write
    |
    V
----------------
ret = v->counter ;
v->counter = (val == v->counter) ? new: v->counter;
-------------
    ^
    |___ read
*/
static inline int atomic_cmpxchg(atomic_t *v, int val, int new)
{
    return arch_cmpxchg(&(v->counter), val, new);
}


#endif
