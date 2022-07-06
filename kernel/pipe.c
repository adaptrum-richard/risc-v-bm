#include "pipe.h"
#include "file.h"
#include "types.h"
#include "slab.h"
#include "page.h"
#include "spinlock.h"
#include "memlayout.h"
#include "wait.h"
#include "vm.h"

int pipealloc(struct file **f0, struct file **f1)
{
    struct pipe *pi;
    char *data = NULL;

    pi = 0;
    *f0 = *f1 = 0;
    if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
        goto bad;
    if ((pi = (struct pipe *)kmalloc(sizeof(struct pipe))) == 0)
        goto bad;
    if((data = (char *)get_free_page()) == NULL)
        goto bad;

    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
    pi->data = data;
    pi->wait_head.head.next = &pi->wait_head.head;
    pi->wait_head.head.prev = &pi->wait_head.head;
    initlock(&pi->wait_head.lock, "pipe_wait");
    initlock(&pi->lock, "pipe");

    (*f0)->type = FD_PIPE;
    (*f0)->readable = 1;
    (*f0)->writable = 0;
    (*f0)->pipe = pi;

    (*f1)->type = FD_PIPE;
    (*f1)->readable = 0;
    (*f1)->writable = 1;
    (*f1)->pipe = pi;
    return 0;

bad:
    if (pi)
        kfree((char *)pi);
    if (*f0)
        fileclose(*f0);
    if (*f1)
        fileclose(*f1);
    if(data)
        free_page((unsigned long)data);
    return -1;
}

int pipewrite(struct pipe *pi, uint64 addr, int n)
{
    int i = 0;

    acquire(&pi->lock);
    while(i < n){
        /*没有开启读权限，写了也没用*/
        if(pi->readopen == 0){
            release(&pi->lock);
            wake_up(&pi->wait_head);
            return -1;
        }
        //pipwrite队列满了,唤醒等待读的进程
        if(pi->nwrite == (pi->nread + PIPESIZE)){
            release(&pi->lock);
            /*唤醒读进程，然后我们自己睡眠，等待读进程把我们唤醒*/
            wake_up(&pi->wait_head);
            wait_event_interruptible(pi->wait_head, (pi->nwrite != pi->nread + PIPESIZE));
            acquire(&pi->lock);
        } else {
            char ch;
            if(spcae_data_copy_in(&ch, addr + i, 1) < 0)
                break;
            pi->data[pi->nwrite++ % PIPESIZE] = ch;
            i++;
        }
    }
    
    release(&pi->lock);
    wake_up(&pi->wait_head);

    return i;
}

int piperead(struct pipe *pi, uint64 addr, int n)
{
    int i;
    char ch;
    acquire(&pi->lock);
    //pipe data是空的, 且写权限开启
    while(pi->nread == pi->nwrite && pi->writeopen){
        release(&pi->lock);
        wait_event_interruptible(pi->wait_head, !(pi->nread == pi->nwrite && pi->writeopen));
        acquire(&pi->lock);
    }

    //piperead数据，读到pipe data为空为止，然后唤醒写进程
    for(i = 0; i < n; i++){
        if(pi->nread == pi->nwrite)
            break;
        ch = pi->data[pi->nread++ % PIPESIZE];
        if(space_data_copy_out(addr + i, &ch, 1) == -1)
            break;
    }
    release(&pi->lock);
    //唤醒写进程
    wake_up(&pi->wait_head);
    
    return i;
}


void pipeclose(struct pipe *pi, int writable)
{
    acquire(&pi->lock);
    if(writable){
        pi->writeopen = 0;
    } else {
        pi->readopen = 0;
    }
    wake_up(&pi->wait_head);
    if(pi->readopen == 0 && pi->writeopen == 0){
        release(&pi->lock);
        free_page((unsigned long)pi->data);
        kfree((char*)pi);
    }else
        release(&pi->lock);
}
