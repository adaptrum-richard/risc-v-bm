#ifndef __BIOPS_H__
#define __BIOPS_H__

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))

#define BITS_PER_BYTE 8
#define BITS_PER_LONG (8 * BITS_PER_BYTE)
#define BITS_TO_LONGS(nr) DIV_ROUND_UP(nr, BITS_PER_LONG)
#define BIT_MASK(nr) (1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr) ((nr) / BITS_PER_LONG)

#define get_bit_in_slot(u64slot, bit) ((u64slot) & (1UL << (bit)))

#define clear_bit_in_slot(u64slot, bit) ((u64slot) = (u64slot) & ~(1UL << (bit)))

#define set_bit_in_slot(u64slot, bit) ((u64slot) = (u64slot) | (1UL << (bit)))

#define __test_and_op_bit_ord(op, mod, nr, addr, ord) \
({ \
    unsigned long __res, __mask; \
    __mask = BIT_MASK(nr); \
    __asm__ volatile(                           \
        "amo" #op ".d" #ord " %0, %2, %1" \
        :"=r"(__res), "+A"(addr[BIT_WORD(nr)]) \
        :"r"(mod(__mask)) \
        :"memory" \
    ); \
    ((__res & __mask) != 0); \
})

#define __op_bit_ord(op, mod, nr, addr, ord) \
    __asm__ volatile( \
        "amo" #op ".d" #ord " zero, %1, %0" \
        : "+A" (addr[BIT_WORD(nr)]) \
        : "r" (mod(BIT_MASK(nr))) \
        : "memory");

#define __test_and_op_bit(op, mod, nr, addr) \
    __test_and_op_bit_ord(op, mod, nr, addr, .aqrl)

#define __op_bit(op, mod, nr, addr) \
    __op_bit_ord(op, mod, nr, addr, )

#define __NOP(x)	(x)
#define __NOT(x)	(~(x))

static inline int test_and_set_bit(int nr, volatile unsigned long *addr)
{
    return __test_and_op_bit(or, __NOP, nr, addr);
}

static inline int test_and_clear_bit(int nr, volatile unsigned long *addr)
{
    return __test_and_op_bit(and, __NOT, nr, addr);
}

/* set No'nr bit to 1 in slot pointed by p */
static inline void set_bit(int nr, volatile unsigned long *addr)
{
    __op_bit(or, __NOP, nr, addr);
}

/* get No'nr bit in slot pointed by p */
static inline int get_bit(unsigned int nr, volatile unsigned long *p)
{
    unsigned nlongs = nr / BITS_PER_LONG;
    unsigned ilongs = nr % BITS_PER_LONG;
    return (p[nlongs] >> ilongs) & 0x1;
}

/* clear No'nr bit in slot pointed by p */
static inline void clear_bit(int nr, volatile unsigned long *addr)
{
    __op_bit(and, __NOT, nr, addr);
}

/* return the first zero bit start from the lowerst bit */
static inline int ctzl(unsigned long x)
{
    return x == 0 ? sizeof(x) * BITS_PER_BYTE : __builtin_ctzl(x);
}

static inline int find_next_bit_helper(unsigned long *p, unsigned long size,
                                       unsigned long start, int invert)
{
    long cur_elem_index, cur_bit,
        max_elem_index, max_bit, cur_bit_value, res = 0;

    max_elem_index = (size - 1) / BITS_PER_LONG;
    cur_elem_index = start / BITS_PER_LONG;
    cur_bit = start % BITS_PER_LONG;
    res = start;

    while (cur_elem_index <= max_elem_index)
    {
        if (cur_elem_index < max_elem_index)
            max_bit = BITS_PER_LONG - 1;
        else
            max_bit = (size - 1) % BITS_PER_LONG;
        for (; cur_bit <= max_bit; cur_bit++, res++)
        {
            cur_bit_value =
                get_bit_in_slot(p[cur_elem_index], cur_bit);
            if (invert ? !cur_bit_value : cur_bit_value)
                return res;
        }
        cur_elem_index++;
        cur_bit = 0;
    }
    return size;
}

/*
 * From lowest bit side, starting from 'start',
 * this function find the first zero bit of the slot pointed by p.
 */
static inline int find_next_zero_bit(unsigned long *p, unsigned long size,
                                     unsigned long start)
{
    return find_next_bit_helper(p, size, start, 1);
}

/*
 * From lowest bit side, starting from 'start',
 * this function find the first bit of the slot pointed by p.
 */
static inline int find_next_bit(unsigned long *p, unsigned long size,
                                unsigned long start)
{
    return find_next_bit_helper(p, size, start, 0);
}

/* From the first 1 bit to the last 1 bit in slot pointed by addr */
#define for_each_set_bit(pos, addr, size)          \
    for ((pos) = find_next_bit((addr), (size), 0); \
         (pos) < (size);                           \
         (pos) = find_next_bit((addr), (size), (pos) + 1))


static inline int ffs(unsigned long x)
{
    int r = 1;

    if (!x)
    return 0;
#if BITS_PER_LONG == 64
    if ((x & 0xffffffff) == 0) {
        r += 32;
        x >>= 32;
    }
#endif
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

static inline unsigned long __ffs(unsigned long word)
{
    int num = 0;

#if BITS_PER_LONG == 64
    if ((word & 0xffffffff) == 0) {
        num += 32;
        word >>= 32;
    }
#endif
    if ((word & 0xffff) == 0) {
        num += 16;
        word >>= 16;
    }
    if ((word & 0xff) == 0) {
        num += 8;
        word >>= 8;
    }
    if ((word & 0xf) == 0) {
        num += 4;
        word >>= 4;
    }
    if ((word & 0x3) == 0) {
        num += 2;
        word >>= 2;
    }
    if ((word & 0x1) == 0)
        num += 1;
    return num;
}

/**
 * __fls - find last (most-significant) set bit in a long word
 * @word: the word to search
 *
 * Undefined if no set bit exists, so code should check against 0 first.
 */
static unsigned long __fls(unsigned long word)
{
    int num = BITS_PER_LONG - 1;

#if BITS_PER_LONG == 64
    if (!(word & (~0ul << 32))) {
        num -= 32;
        word <<= 32;
    }
#endif
    if (!(word & (~0ul << (BITS_PER_LONG-16)))) {
        num -= 16;
        word <<= 16;
    }
    if (!(word & (~0ul << (BITS_PER_LONG-8)))) {
        num -= 8;
        word <<= 8;
    }
    if (!(word & (~0ul << (BITS_PER_LONG-4)))) {
        num -= 4;
        word <<= 4;
    }
    if (!(word & (~0ul << (BITS_PER_LONG-2)))) {
        num -= 2;
        word <<= 2;
    }
    if (!(word & (~0ul << (BITS_PER_LONG-1))))
        num -= 1;
    return num;
 }
 
 static inline unsigned long fls64(unsigned long x)
 {
    if (x == 0)
        return 0;
    return __fls(x) + 1;
 }

#endif
