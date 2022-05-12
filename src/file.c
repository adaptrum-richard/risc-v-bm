#include "param.h"
#include "file.h"
#include "spinlock.h"
#include "printk.h"
#include "fs.h"
#include "log.h"

struct devsw devsw[NDEV];

struct ftable {
    struct spinlock lock;
    struct file file[NFILE];
};

struct ftable ftable;

void fileinit(void)
{
    initlock(&ftable.lock, "ftable");
}

struct file *filealloc(void)
{
    struct file *f;

    acquire(&ftable.lock);
    for(f = ftable.file; f < ftable.file + NFILE; f++){
        if(f->ref == 0){
            f->ref = 1;
            release(&ftable.lock);
            return f;
        }
    }
    release(&ftable.lock);
    return 0;
}

void fileclose(struct file *f)
{
    struct file ff;
    acquire(&ftable.lock);
    if(f->ref < 1)
        panic("fileclose");
    if(--f->ref > 0){
        release(&ftable.lock);
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if(ff.type == FD_INODE){
        log_begin_op();
        iput(ff.ip);
        log_end_op();
    }
}

struct file *filedup(struct file *f)
{
    acquire(&ftable.lock);
    if(f->ref < 1)
        panic("filedup\n");

    f->ref++;
    release(&ftable.lock);
    return f;
}

int fileread(struct file *f, uint64 addr, int n)
{
    int r = 0;
    if(f->readable == 0)
        return -1;
    if(f->type == FD_INODE){
        ilock(f->ip);
        if((r = readi(f->ip, addr, f->off, n)) > 0)
            f->off += r;
        iunlock(f->ip);
    } else if (f->type == FD_DEVICE) {
        if(f->major < 0 || f->major >= NDEV || !devsw[f->major].read)
            return -1;
        r = devsw[f->major].read(addr, n);
    }
    

    return r;
}

int filewrite(struct file *f, uint64 addr, int n)
{
    int r, ret = 0;

    if(f->writable == 0)
        return -1;
    
    if(f->type == FD_INODE){
        int max = ((MAXOPBLOCKS-1-1-2) / 2) * BSIZE;
        int i = 0;
        while(i < n){
            int n1 = n - i;
            if(n1 > max)
                n1 = max;
            
            log_begin_op();
            ilock(f->ip);
            if((r = writei(f->ip, addr + i, f->off, n1)) > 0)
                f->off += r;
            iunlock(f->ip);
            log_end_op();

            if(r != n1){
                //writei error;
                break;
            }
            i += r;
        }
        ret = (i==n ? n : -1);
    }else if(f->type == FD_DEVICE){
        if(f->major < 0 || f->major >= NDEV || !devsw[f->major].write)
            return -1;
        ret = devsw[f->major].write(addr, n);
    }

    return ret;
}
