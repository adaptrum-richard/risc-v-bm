#ifndef __CMPCHG_H__
#define __CMPCHG_H__

#include "barrier.h"
#define ____xchg_relaxed(ptr, new, size)  \
({  \
    typeof(ptr) __ptr = (ptr);          \
    typeof(new) __new = (new);      \
    typeof(*(ptr)) __ret;           \
    switch(size){                  \
        case 4:                     \
            __asm__ volatile(   \
                "amoswap.w %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
        case 8:                 \
            __asm__ volatile(   \
                "amoswap.d %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
    }                           \
    __ret;                      \
})

#define __xchg_relaxed(ptr, x)  \
({  \
    typeof(*(ptr)) _x_ = (x);   \
    (typeof(*(ptr))) ____xchg_relaxed((ptr), _x_, sizeof(*(ptr)));   \
})

#define ____xchg_acquire(ptr, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(new) __new = (new); \
    typeof(*(ptr)) __ret; \
    switch(size) {  \
        case 4: \
            __asm__ volatile(   \
                "amoswap.w.aq %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
        case 8: \
            __asm__ volatile(   \
                "amoswap.d.aq %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
    }                           \
    __ret;                      \
})

#define __xchg_acquire(ptr, x)    \
({  \
    typeof(*(ptr)) _x_ = (x); \
    (typeof(*(ptr))) ____xchg_acquire((ptr), _x_, sizeof(*(ptr))); \
})


#define ____xchg_release(ptr, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(new) __new = (new); \
    typeof(*(ptr)) __ret; \
    switch(size) {  \
        case 4: \
            __asm__ volatile(   \
                "amoswap.w.rl %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
        case 8: \
            __asm__ volatile(   \
                "amoswap.d.rl %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
    }                           \
    __ret;                      \
})

#define __xchg_release(ptr, x)    \
({  \
    typeof(*(ptr)) _x_ = (x); \
    (typeof(*(ptr))) ____xchg_release((ptr), _x_, sizeof(*(ptr))); \
})

#define ____xchg(ptr, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(new) __new = (new); \
    typeof(*(ptr)) __ret; \
    switch(size) {  \
        case 4: \
            __asm__ volatile(   \
                "amoswap.w.aqrl %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
        case 8: \
            __asm__ volatile(   \
                "amoswap.d.aqrl %0, %2, %1\n" \
                :"=r"(__ret), "+A"(*__ptr) \
                :"r" (__new)    \
                :"memory"       \
            );                  \
        break;                  \
    }                           \
    __ret;                      \
})

#define __xchg(ptr, x)    \
({  \
    typeof(*(ptr)) _x_ = (x); \
    (typeof(*(ptr))) ____xchg((ptr), _x_, sizeof(*(ptr))); \
})

#define ____cmpxchg_relaxed(ptr, old, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(*(ptr)) __old = (old); \
    typeof(*(ptr)) __new = (new); \
    typeof(*(ptr)) __ret; \
    register unsigned int __rc; \
    switch(size) { \
    case 4: \
        __asm__ volatile (  \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    case 8: \
        __asm__ volatile (  \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    } \
    __ret; \
})

#define __cmpxchg_relaxed(ptr, o, n) \
({  \
    typeof(*(ptr)) _o_ = (o); \
    typeof(*(ptr)) _n_ = (n); \
    (typeof(*(ptr))) ____cmpxchg_relaxed((ptr), _o_, _n_, sizeof(*(ptr))); \
})

#define ____cmpxchg_acquire(ptr, old, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(*(ptr)) __old = (old); \
    typeof(*(ptr)) __new = (new); \
    typeof(*(ptr)) __ret; \
    register unsigned int __rc; \
    switch(size) { \
    case 4: \
        __asm__ volatile (  \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence r, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    case 8: \
        __asm__ volatile (  \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence r, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    } \
    __ret; \
})

#define __cmpxchg_acquire(ptr, o, n) \
({  \
    typeof(*(ptr)) _o_ = (o); \
    typeof(*(ptr)) _n_ = (n); \
    (typeof(*(ptr))) ____cmpxchg_acquire((ptr), _o_, _n_, sizeof(*(ptr))); \
})

#define ____cmpxchg_release(ptr, old, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(*(ptr)) __old = (old); \
    typeof(*(ptr)) __new = (new); \
    typeof(*(ptr)) __ret; \
    register unsigned int __rc; \
    switch(size) { \
    case 4: \
        __asm__ volatile (  \
            "fence rw,  w\n"    \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence r, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    case 8: \
        __asm__ volatile (  \
            "fence rw,  w\n"    \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence r, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    } \
    __ret; \
})

#define __cmpxchg_release(ptr, o, n) \
({  \
    typeof(*(ptr)) _o_ = (o); \
    typeof(*(ptr)) _n_ = (n); \
    (typeof(*(ptr))) ____cmpxchg_release((ptr), _o_, _n_, sizeof(*(ptr))); \
})

#define ____cmpxchg(ptr, old, new, size) \
({  \
    typeof(ptr) __ptr = (ptr); \
    typeof(*(ptr)) __old = (old); \
    typeof(*(ptr)) __new = (new); \
    typeof(*(ptr)) __ret; \
    register unsigned int __rc; \
    switch(size) { \
    case 4: \
        __asm__ volatile (  \
            "fence rw,  w\n"    \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w.rl %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence rw, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    case 8: \
        __asm__ volatile (  \
            "fence rw,  w\n"    \
            "0: lr.w %0, %2\n" \
            "   bne %0, %z3, lf\n" \
            "   sc.w.rl %1, %z4, %2\n" \
            "   bnez %1, 0b\n" \
            "   fence rw, rw\n" \
            "1:\n"  \
            : "=&r"(__ret), "=&r"(__rc), "+A"(*__ptr) \
            : "rJ"((long)__old), "rJ"(__new) \
            : "memory"  \
        ); \
        break; \
    } \
    __ret; \
})

#define __cmpxchg(ptr, o, n) \
({  \
    typeof(*(ptr)) _o_ = (o); \
    typeof(*(ptr)) _n_ = (n); \
    (typeof(*(ptr))) ____cmpxchg((ptr), _o_, _n_, sizeof(*(ptr))); \
})


#define arch_xchg_relaxed       __xchg_relaxed
#define arch_xchg_acquire       __xchg_acquire
#define arch_xchg_release       __xchg_release

#define arch_xchg               __xchg
#define arch_cmpxchg_relaxed    __cmpxchg_relaxed
#define arch_cmpxchg_acquire    __cmpxchg_acquire
#define arch_cmpxchg_release    __cmpxchg_release
#define arch_cmpxchg            __cmpxchg

#endif
