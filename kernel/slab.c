#include "slab.h"
#include "page.h"
#include "types.h"
#include "string.h"
#include "mm.h"
#include "spinlock.h"
#include "debug.h"

/*
 * slob机制设计思路
 * 1. 并没有按照对象的大小来分配一个特定的slab cache
 * 任何size的slab对象都可以 拥挤到一个page里,
 * 即一个page里可以同时存在不同大小的对象。
 *
 * 2. 没有pre-cpu的缓存和多核share缓存的思想。
 * 3. 需要分配对象时，就从一个有空闲空间的page里
 * 查找有没有合适的空间来存储。
 *
 * 4. 把一个page分成一个个小格子单元，一个小格子
 *  单元占用2个字节。
 *
 *  5. struct page里有两个成员配合slob分配器，一个
 *  是freelist指向当前空闲空间的地址，一个是
 *  slob_left_units表示剩余多少个小格子。
 *  空闲空间的第一个小格子用来存储 对象的大小size，
 *  第二小格子用来指向下一个空闲空间的起始地址next
 */

static LIST_HEAD(free_slob_list);

struct spinlock slob_lock;

typedef short slobidx_t;

struct slob_block
{
    slobidx_t units;
};
typedef struct slob_block slob_t;

/* slob用小格子来计算空间，一个小格子是2个字节*/
#define SLOB_UNIT sizeof(slob_t)

/* 计算占用多少个小格子 */
#define SLOB_UNITS(size) DIV_ROUND_UP(size, SLOB_UNIT)

static void set_slob(slob_t *s, slobidx_t size, slob_t *next)
{
    slob_t *base = (slob_t *)((unsigned long)s & PAGE_MASK);
    slobidx_t offset = next - base;

    /* 第一个小格子存储空闲块的大小
     * 第二小格子存储下一个空间块的起始地址
     */
    if (size > 1){
        s[0].units = size;
        s[1].units = offset;
    }
    else
        s[0].units = -offset;
}

/*
 * 下一个空闲块的起始地址
 */
static slob_t *slob_next(slob_t *s)
{
    slob_t *base = (slob_t *)((unsigned long)s & PAGE_MASK);
    slobidx_t next;

    if (s[0].units < 0)
        next = -s[0].units;
    else
        next = s[1].units;
    return base + next;
}

/*
 * 返回空闲块的大小
 */
static slobidx_t slob_units(slob_t *s)
{
    if (s->units > 0)
        return s->units;
    return 1;
}

/*
 * 判断是否最后一个空闲块
 */
static int slob_last(slob_t *s)
{
    return !((unsigned long)slob_next(s) & ~PAGE_MASK);
}

static void *slob_page_alloc(struct page *page, size_t size, int align)
{
    slob_t *cur;
    int object_size = SLOB_UNITS(size);

    for (cur = page->freelist;; cur = slob_next(cur)){
        /* 这个page剩余的空间*/
        slobidx_t avail = slob_units(cur);

        /* 有足够的空间容纳这个object*/
        if (avail >= object_size){
            slob_t *next;

            next = slob_next(cur);
            if (avail == object_size){
                /* 空间正好，则直接使用这个空闲
                 * 空间
                 *
                 *freelist指向下一个空间空间next
                 */
                page->freelist = next;
            }
            else{
                /*
                 * 空间比object_size大，说明要留出hole
                 *
                 * freelist指向这个hole
                 *
                 * 这个hole的第一个小格子表示hole的
                 * 空间
                 * 第二小格子表示下一个空闲空间
                 * next的地址
                 */
                page->freelist = cur + object_size;
                set_slob(cur + object_size,
                         avail - object_size, next);
            }

            page->slob_left_units -= object_size;
            return cur;
        }
        if (slob_last(cur))
            return NULL;
    }
}

void slob_free(void *block, int size)
{
    struct page *page;
    slob_t *b = (slob_t *)block;
    slob_t *prev, *next;
    slobidx_t object_size;
    slobidx_t free_size;

    page = virt_to_page((unsigned long)block);

    object_size = free_size = SLOB_UNITS(size);

    /* 直接释放这个页面*/
    if (page->slob_left_units + object_size == SLOB_UNITS(PGSIZE)){
        if (PageSlab(page)){
            list_del(&page->lru);
            ClearPageSlab(page);
        }
        pr_slab_debug("free page, pa addr 0x%lx\n", (unsigned long)b);
        free_page((unsigned long)b);

        return;
    }

    /* free it now */
    page->slob_left_units += object_size;

    /* 释放地址小于freelist的情况 */
    if (b < (slob_t *)page->freelist){
        if (b + object_size == page->freelist){
            object_size += slob_units(page->freelist);
            page->freelist = slob_next(page->freelist);
        }

        /* 在释放b的地方，第一个小格子设置空闲的大小
         * 第二小格子用来指向下一个空闲空间的
         * 起始地址
         */
        set_slob(b, free_size, page->freelist);
        page->freelist = b;
    }
    else{
        /* 释放地址大于freelist的情况*/
        prev = page->freelist;
        next = slob_next(prev);

        /* 找到一块next地址大于 释放地址的空闲块*/
        while (b > next){
            prev = next;
            next = slob_next(prev);
        }

        if (!slob_last(prev) && b + object_size == next){
            free_size += slob_units(next);
            set_slob(b, free_size, slob_next(next));
        }
        else
            set_slob(b, free_size, next);

        if (prev + slob_units(prev) == b){
            free_size = slob_units(b) + slob_units(prev);
            set_slob(prev, free_size, slob_next(b));
        }
        else
            set_slob(prev, slob_units(prev), b);
    }
}

static void *slob_alloc_new_page(int order)
{
    return (void *)get_free_pages(order);
}

static inline void check_alloc_scope(unsigned long start_pa, 
    unsigned long alloc_pa, size_t size)
{
    unsigned long end_pa = start_pa + PGSIZE;
    unsigned long alloc_pa_end = size + alloc_pa;
    if(alloc_pa < start_pa || alloc_pa_end > end_pa)
        panic("%s %s: [0x%lx+%lx]  beyond the scope of [0x%lx-0x%lx]\n", __func__, __LINE__, 
                start_pa, size , start_pa, alloc_pa_end);
}

void *slob_alloc(size_t size, int align)
{
    struct page *page;

    slob_t *b = NULL;
    acquire(&slob_lock);
    list_for_each_entry(page, &free_slob_list, lru){
        if (page->slob_left_units < SLOB_UNITS(size))
            continue;

        b = slob_page_alloc(page, size, align);
        if (!b)
            continue;
        
        check_alloc_scope(page_to_address(page), (unsigned long)b, size);
        break;
    }
    release(&slob_lock);

    if (!b){
        b = slob_alloc_new_page(0);
        if (!b)
            return NULL;
        SetPageSlab(page);
        pr_slab_debug("slab new page,order:%d, pa:0x%lx, page addr:0x%lx\n", 
            0, (unsigned long)b, (unsigned long)page);
        page->slob_left_units = SLOB_UNITS(PGSIZE);
        page->freelist = b;
        INIT_LIST_HEAD(&page->lru);
        set_slob(b, SLOB_UNITS(PGSIZE), b + SLOB_UNITS(PGSIZE));
        list_add(&page->lru, &free_slob_list);
        acquire(&slob_lock);
        b = slob_page_alloc(page, size, align);
        release(&slob_lock);
    }

    memset(b, 0, size);

    return b;
}

void *kmem_cache_zalloc(struct kmem_cache *s)
{
    void *b;

    if (s->size < PGSIZE)
        b = slob_alloc(s->size, s->align);
    else
        b = slob_alloc_new_page(get_order(s->size));

    if (b)
        memset(b, 0, s->size);

    return b;
}

/*
 * kmalloc - 分配内存块，并内容清0
 */
void *kmalloc(size_t size)
{
    int align = sizeof(unsigned long long);
    unsigned int *b;
    void *ret;
    struct page *page;
    pr_slab_debug("enter\n");
    size = ALIGN(size, sizeof(unsigned int));

    if (size < PGSIZE - align){

        b = slob_alloc(size + align, align);
        if (!b)
            return NULL;

        *b = size;
        ret = (void *)b + align;
        pr_slab_debug("alloc addr: 0x%lx, size: 0x%lx\n", (unsigned long)ret, 
            size);
    }
    else{
        unsigned int order = get_order(size);

        ret = slob_alloc_new_page(order);
        memset(ret, 0, size);
        page = virt_to_page((unsigned long)ret);
        pr_slab_debug("malloc page,order:%d, pa:0x%lx, page addr:0x%lx\n", 
            order, ret, (unsigned long)page);
        page->slob_left_units = order;
    }
    pr_slab_debug("exit\n");
    return ret;
}

/*
 * kfree - 释放内存块
 */
void kfree(const void *block)
{
    struct page *page;

    page = virt_to_page((unsigned long)block);
    if (PageSlab(page)){
        int align = sizeof(unsigned long long);
        unsigned int *m = (unsigned int *)(block - align);
        acquire(&slob_lock);
        slob_free(m, *m + align);
        release(&slob_lock);
    }
    else
        free_pages((unsigned long)block, page->slob_left_units);
}

void kmem_cache_free(struct kmem_cache *s, void *b)
{
    if (s->size < PGSIZE)
        slob_free(b, s->size);
    else
        free_pages((unsigned long)b,
                   get_order(s->size));
}

int __kmem_cache_create(struct kmem_cache *s, unsigned long flags)
{
    s->flags = flags;

    return 0;
}

struct kmem_cache kmem_cache_boot = {
    .name = "kmem_cache_boot",
    .size = sizeof(struct kmem_cache),
};

void kmem_cache_init(void)
{
    boot_kmem_cache = &kmem_cache_boot;
    initlock(&slob_lock, "slob");
}

struct kmem_cache *boot_kmem_cache;
LIST_HEAD(g_slab_caches);

struct kmem_cache * kmem_cache_create(const char *name, 
    size_t size, size_t align, unsigned long flags)
{
    struct kmem_cache *s;
    int ret;

    /* create a kmem_cache */
    s = kmem_cache_zalloc(boot_kmem_cache);
    if (!s)
        goto out;

    s->name = name;
    s->object_size = s->size = size;
    s->align = align;

    ret = __kmem_cache_create(s, flags);
    if (ret)
        goto out_free_cache;

    s->refcount = 1;
    list_add(&s->list, &g_slab_caches);

    return s;

out_free_cache:
    kmem_cache_free(boot_kmem_cache, s);
out:
    return NULL;
}

unsigned long slob_array[1000];
void test_slob(void)
{
    for(int i = 0 ; i< 500; i++){
        slob_array[i] = (unsigned long)kmalloc((i+1)*4);
        if(slob_array[i] == 0)
            panic("%s: BUG1\n", __func__);
        memset((void*)slob_array[i], i%255, (i+1)*4);
    }
    unsigned char *p;
    for(int i = 0 ; i< 500; i++){
        p = (unsigned char*)slob_array[i];
        for(int j = 0; j < (i+1)*4; j++){
            if(p[j] != (i%255)){
                panic("%s: BUG2 p[%d] = 0x%ld\n", __func__, j, p[j]);
            }
        }
        kfree((void*)slob_array[i]);
    }
}
