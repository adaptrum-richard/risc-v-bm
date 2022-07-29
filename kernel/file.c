#include "param.h"
#include "file.h"
#include "spinlock.h"
#include "printk.h"
#include "fs.h"
#include "log.h"
#include "proc.h"
#include "vm.h"
#include "mm.h"
#include "debug.h"
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
        return;
    }
    ff = *f;
    f->ref = 0;
    f->type = FD_NONE;
    release(&ftable.lock);

    if(ff.type == FD_INODE){
        log_begin_op();
        iput(ff.ip);
        log_end_op();
    } else if(ff.type == FD_PIPE)
        pipeclose(ff.pipe, ff.writable);
    else if(ff.type == FD_SOCK)
        sockclose(ff.sock);
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
    } else if( f->type == FD_PIPE) 
        r = piperead(f->pipe, addr, n);
    else if( f->type == FD_SOCK)
        r = sockread(f->sock, addr, n);
    else 
        panic("file read\n");
    

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
    } else if(f->type == FD_PIPE)
        ret = pipewrite(f->pipe, addr, n);
    else if (f->type == FD_SOCK)
        ret = sockwrite(f->sock, addr, n);
    else 
        panic("file write\n");
    return ret;
}

// Get metadata about file f.
// addr is a user virtual address, pointing to a struct stat.
int filestat(struct file *f, uint64 addr)
{
    struct stat st;
    if(f->type == FD_INODE || f->type == FD_DEVICE){
        ilock(f->ip);
        stati(f->ip, &st);
        iunlock(f->ip);
        if(space_data_copy_out(addr, (char *)&st, sizeof(st)) < 0)
            return -1;
        return 0;
    }
    return -1;
}
