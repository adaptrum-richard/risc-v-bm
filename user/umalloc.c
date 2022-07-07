#include "umalloc.h"
#ifdef UMALLOCK_TEST
#include <stdio.h>
#include <unistd.h> //  brk and sbrk
#else
#include "kernel/types.h"
#endif
/*
maybe sbrk和brk原理参考https://www.jianshu.com/p/4ddf472226cc
*/

block_metadata_t *_block_head = NULL;

/*从b中切割一个size下来，newb指向剩余的空间的起始地址，
b则是新分配的内存的描述符，b->size存放新分配的内存大小。
参数size = addr3 - add2
1.split_block前：

addr1               addr2                 addr3          addr4
|                   |                     |               |
V                   V                     V               V
-----------------------------------------------------------
|      |      |     |                     |               |  
| next | prev |size |                     |               |
|      |      |     |                     |               |
-----------------------------------------------------------
|<--  结构体大小  -->|<---------------- b->szie  --------->|                 
                    
b->size = BLOCK_META_SIZE + (addr4-addr1)
b指向内存起始地址 addr1

2. split_block后
newb 指向地址addr3 , newb->size 为 addr4 - addr3 - BLOCK_META_SIZE
b 指向 addr1，b->size 为 addr3-addr2即为参数size

*/
block_metadata_t *split_block(block_metadata_t *b, 
    size_t size)
{
    block_metadata_t *newb;
    newb = (block_metadata_t *)((unsigned long)b + 
                    BLOCK_META_SIZE + size);
    newb->size = b->size - (BLOCK_META_SIZE + size);
    b->size = size;
    return newb;
}

void block_stats(char *stage)
{

    printf("\t%s program break: 0x%lx\n", stage, (unsigned long)sbrk(0));
    block_metadata_t *ptr = _block_head;
    while(ptr){
        printf("\t\tblock addr:0x%lx, size: 0x%x\n", (unsigned long)ptr, ptr->size);
        ptr = ptr->next;
    }

}

/*向block链表中添加一个free的block，注意链表是按顺序排列的*/
void add_to_freelist(block_metadata_t *freeb)
{
#ifdef UMALLOCK_TEST
    printf( "Adding 0x%lx with size 0x%x to free list \n", (unsigned long)freeb,
            freeb->size);
#endif
    freeb->next = freeb->prev = NULL;

    /*block head为空，或freeb地址小于block头*/
#if 0
    if(!_block_head || GE_ADDR(_block_head, freeb)){
        if(_block_head)
            _block_head->prev = freeb;
        freeb->next = _block_head;
        _block_head = freeb;
        return;
    }
#else
   int found = 0;
    if(!_block_head)
        _block_head = freeb;
    else {
        if(LE_ADDR(freeb, _block_head)){
            freeb->next = _block_head;
            _block_head->prev = freeb;
            _block_head = freeb;
            printf("%s %d\n", __func__, __LINE__);
        }else {
            block_metadata_t *ptr;
            for(ptr = _block_head; ptr; ptr = ptr->next){
                if(ptr->next){
                    printf("ptr->next = 0x%lx, freeb = 0x%lx\n", (unsigned long)ptr->next, (unsigned long)freeb);
                    if(GT_ADDR(ptr->next, freeb)){
                        ptr->next->prev = freeb;
                        freeb->next = ptr->next;
                        freeb->prev = ptr;
                        ptr->next = freeb;
                        printf("%s %d\n", __func__, __LINE__);
                        break;
                    }
                }
                if(ptr->next == NULL){
                    ptr->next = freeb;
                    freeb->prev = ptr;
                    printf("%s %d\n", __func__, __LINE__);
                    break;
                }
            }
        }
    }
#endif
    block_stats("遍历");
#if 0
    block_metadata_t *ptr = _block_head;

    while(ptr->next && LE_ADDR(ptr->next, freeb))
        ptr = ptr->next;
    /*注意这里ptr->next可能为空*/
    freeb->next = ptr->next;
    freeb->prev = ptr;
    if(ptr->next != NULL){
        (ptr->next)->prev = freeb;
        ptr->next = freeb;
    }
#endif
    return;
}

void remove_from_freelist(block_metadata_t *b)
{
    if(!b->prev)
        if(b->next)
            _block_head = b->next;
        else
            _block_head = NULL;
    else 
        b->prev->next = b->next;
    
    if(b->next)
        b->next->prev = b->prev;

}

#ifdef UMALLOCK_TEST
void *_malloc(size_t size)
#else 
void *malloc(size_t size)
#endif
{
    block_metadata_t *ptr = _block_head;
    while(ptr){
        /*查找一个和想分配内存size一样大的block
        如果没有一样大的block就分配一个
        */
        if(ptr->size >= size){
            remove_from_freelist(ptr);
            if(ptr->size == size){
                printf("malloc block:0x%lx, size:0x%lx\n", (unsigned long)(ptr), size);
                return BLOCK_MEM(ptr);
            }
            block_metadata_t *newb = split_block(ptr, size);
            add_to_freelist(newb);
            printf("malloc block:0x%lx, size:0x%lx\n", (unsigned long)(ptr), size);
            return BLOCK_MEM(ptr);
        }
        ptr = ptr->next;
    }
    //printf("%s %d: ALLOC_UNIT = 0x%lx, size = %d\n", __func__, __LINE__, ALLOC_UNIT, size);
    size_t alloc_size;
    if(size >= ALLOC_UNIT)
        alloc_size = PAGE_UP(size + BLOCK_META_SIZE);
    else 
        alloc_size = ALLOC_UNIT;
    
    /*分配虚拟内存*/
    ptr = (block_metadata_t *)sbrk(alloc_size);
    if(!ptr) {
        printf("failed to alloc 0x%lx\n", alloc_size);
        return NULL;
    }
#ifdef UMALLOCK_TEST
    //printf("alloc addr 0x%lx, size = 0x%lx, end addr = 0x%lx\n", (unsigned long)ptr, alloc_size, (unsigned long)ptr + alloc_size);

#endif

    ptr->next = ptr->prev = NULL;
    /*剩余的可用内存空间记录在size中。不包括block_metadata的大小*/
    ptr->size = alloc_size - BLOCK_META_SIZE;

    if(alloc_size > size + BLOCK_META_SIZE){
         
        block_metadata_t *newb = split_block(ptr, size);
         
        add_to_freelist(newb);
         
    }
    printf("malloc block:0x%lx, size:0x%lx\n", (unsigned long)(ptr), size);
    return BLOCK_MEM(ptr);
}

void scan_and_merge()
{
    unsigned long curr_addr, next_addr;
    unsigned long addr;
    block_metadata_t *tb;
    block_metadata_t *curr;
    
    //merge
retry:
    curr = _block_head;
    while(curr->next){

        curr_addr = (unsigned long)curr;
        next_addr = (unsigned long)curr->next;
        addr = curr_addr + BLOCK_META_SIZE + curr->size;
        
        if(addr == next_addr){
            curr->size += curr->next->size + BLOCK_META_SIZE;
            //curr->next = curr->next->next;
            tb = curr->next->next;
            if(tb){
                //删除block:curr->next
                curr->next = tb;
                tb->prev = curr;
                goto retry;
            }
            else{
                curr->next = NULL;
                break;
            }
        }
        curr = curr->next;
    }
    unsigned long program_break = (unsigned long)sbrk(0);
    if(program_break == 0){
        printf("failed to get program break\n");
        return;
    }
#ifdef RISC_V_BM
    /*对于RISC_V_BM, 需要找到最后一个free的block，并且这个block的size是大于PAGE - BLOCK_META_SIZE*/
    curr = _block_head;
    while(curr->next){
        curr = curr->next;
    }
    
    curr_addr = (unsigned long)curr;
    printf("%s %d: curraddr = 0x%lx, size = 0x%lx, MIN_DEALLOC = 0x%lx, end = 0x%lx, program_break = 0x%lx\n", 
        __func__, __LINE__, 
        curr_addr, curr->size,MIN_DEALLOC ,curr_addr + curr->size + BLOCK_META_SIZE, program_break);
    if(((curr_addr + curr->size + BLOCK_META_SIZE) == program_break) && (curr->size+ BLOCK_META_SIZE)>= MIN_DEALLOC){
         printf("%s %d\n", __func__, __LINE__);
        if((curr_addr & (MIN_DEALLOC - 1)) == 0) {//PAGE对齐
            printf("%s %d\n", __func__, __LINE__);
            remove_from_freelist(curr);
            if(curr == _block_head){
                
                _block_head = NULL;
            }
            if(brk(curr) != 0)
                printf("error free memory!");
        } else {
            int count = (curr->size + BLOCK_META_SIZE) / MIN_DEALLOC;
            int remainder = (curr->size + BLOCK_META_SIZE) % MIN_DEALLOC;
            
            if(remainder <= BLOCK_META_SIZE && count == 1){
                /*do nothing
                此情况不需要做任何事
                */
                
            } else if( count > 1 && remainder <= BLOCK_META_SIZE){
                unsigned long free_addr = PAGE_UP(curr_addr);
                /*保留一个页，其余释放掉,更新size*/
                curr->size -= (count-1)*MIN_DEALLOC;
                printf("1+++++++++++free addr = %lx, size = %lx\n", free_addr, (count-1)*MIN_DEALLOC);
                if(brk((void*)free_addr) != 0)
                    printf("error free memory!");
            } else if( count > 1 && remainder > BLOCK_META_SIZE){
                unsigned long free_addr = PAGE_UP(curr_addr);
                curr->size -= count*MIN_DEALLOC;
                printf("2+++++++++++free addr = %lx, size = %lx\n", free_addr, (count-1)*MIN_DEALLOC);
                if(brk((void *)free_addr) != 0)
                    printf("error free memory!");
            }
        }
    }
#else
    curr_addr = (unsigned long)curr;

    
    if(((curr_addr + curr->size + BLOCK_META_SIZE) == program_break)
            && curr->size >= MIN_DEALLOC ) {
        remove_from_freelist(curr);
#ifdef UMALLOCK_TEST
        printf("free addr:0x%lx, size = 0x%lx\n", (unsigned long)curr, 
                (unsigned long)(curr->size + BLOCK_META_SIZE));
#endif
        if(brk(curr) != 0)
            printf("error free memory!");
    }
#endif
}

#ifdef UMALLOCK_TEST
void _free(void *addr)
#else 
void free(void *addr)
#endif
{


    block_metadata_t *block_addr = BLOCK_HEADER(addr);

#ifdef UMALLOCK_TEST
    block_stats("free前:");
#endif
    add_to_freelist(block_addr);
#ifdef UMALLOCK_TEST
    block_stats("free后:");
#endif


    scan_and_merge();
  #ifdef UMALLOCK_TEST
    //block_stats("merge 后:");
#endif  

}

void cleanup()
{
    printf("Cleaning up memory ...\n");
    if( _block_head != NULL ) {
        if(brk(_block_head) != 0 ) {
            printf("Failed to cleanup memory");
        }
    }
    _block_head = NULL;
}

#ifdef UMALLOCK_TEST
int main()
{
    printf("block metadata size: 0x%x\n", (unsigned int)BLOCK_META_SIZE);
    block_stats("malloc start ========================");
    int *p1 =_malloc(1000);
    int *p2 = _malloc(2000);
    int *p3 = _malloc(3000);

    block_stats("111111=========================");
    printf("mallco end ================================\n");

    printf("p1 free =============================\n");
    _free(p1);
    printf("p2 free =============================\n");
    _free(p3);
     printf("p3 free =============================\n");
    _free(p2);
    printf("clean =============================\n");


    //block_stats("clean befre");
    cleanup();

    return 0;
}
#endif
