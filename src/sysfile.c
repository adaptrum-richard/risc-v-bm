#include "types.h"
#include "log.h"
#include "file.h"
#include "fs.h"
#include "stat.h"
#include "printk.h"
#include "string.h"
#include "fcntl.h"
#include "proc.h"
#include "string.h"

static struct inode *create(char *path, short type, 
    short major, short minor)
{
    struct inode *ip, *dp;
    char name[DIRSIZ];
    
    /*找到此文件父目录的inode*/
    if((dp = nameiparent(path, name)) == 0)
        return 0;

    ilock(dp);
    /*在父目录中搜索是否存在此文件*/
    if((ip = dirlookup(dp, name, 0)) != 0){
        /*如果找到对应的文件则返回文件对应的inode*/
        iunlockput(dp);
        ilock(ip);
        if(type == T_FILE && (ip->type == T_FILE || ip->type == T_DEVICE)){
            return ip;
        }
        /*打开的文件存在，但是是一个目录，出问题了。
        这里是否应该处理type为目录的情况？
        */
        iunlockput(ip);
        return 0;
    }
    /*到这里说明，没有找到要找的文件，则分配一个inode*/
    if((ip = ialloc(dp->dev, type)) == 0)
        panic("create： ialloc\n");
    
    /*初始化inode*/
    ilock(ip);
    ip->major = major;
    ip->minor = minor;
    ip->nlink = 1;
    iupdate(ip);
    
    /*处理创建目录情况*/
    if(type == T_DIR){
        /*父目录的nlink加1*/
        dp->nlink++;
        iupdate(dp);
        /*创建此目录下的dots*/
        if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0){
            panic("create dots\n");
        }
    }

    /*创建文件或目录*/
    if(dirlink(dp, name, ip->inum) < 0)
        panic("create: dirlink\n");
    
    iunlockput(dp);
    return ip;
}

static int fdalloc(struct file *f)
{
    int fd;
    struct task_struct *p = current;
    for(fd = 0; fd < NOFILE; fd++){
        if(p->ofile[fd] == 0){
            p->ofile[fd] = f;
            return fd;
        }
    }
    return -1;
}

uint64 __sys_open(const char *pathname, int omode)
{
    char path[MAXPATH] = {0};
    int fd;
    struct file *f;
    struct inode *ip;

    if(!pathname || strlen(pathname) > MAXPATH)
        return -1;
    
    strncpy(path, pathname, MAXPATH);

    log_begin_op();
    
    if(omode & O_CREATE){
        ip = create(path, T_FILE, 0, 0);
        if(ip == 0){
            log_end_op();
            return -1;
        }
    } else {
        if((ip = namei(path)) == 0){
            log_end_op();
            return -1;
        }
        
        ilock(ip);
        if(ip->type == T_DIR && omode != O_RDONLY){
            iunlockput(ip);
            log_end_op();
            return -1;
        }
    }

    if(ip->type == T_DEVICE && (ip->major < 0 || ip->major >= NDEV)){
        iunlockput(ip);
        log_end_op();
        return -1;
    }
    
    if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
        if(f)
            fileclose(f);
        iunlockput(ip);
        log_end_op();
        return -1;
    }

    if(ip->type == T_DEVICE){
        f->type = FD_DEVICE;
        f->major = ip->major;
    } else {
        f->type = FD_INODE;
        f->off = 0;
    }
    
    f->ip = ip;
    f->readable = !(omode & O_WRONLY);
    f->writable = (omode & O_WRONLY) || (omode & O_RDWR);

    if((omode & O_TRUNC) && ip->type == T_FILE){
        itrunc(ip);
    }
    iunlock(ip);
    log_end_op();
    return 0;
}

uint64 __sys_read(int fd, void *buf, int count)
{
    struct file *f = current->ofile[fd];
    if(buf == 0 || count < 0 || f == 0)
        return -1;
    return fileread(f, (uint64)buf, count);
}

uint64 __sys_write(int fd, const void *buf, int count)
{
    struct file *f = current->ofile[fd];
    if(buf == 0 || count < 0 || f == 0)
        return -1;
    return filewrite(f, (uint64)buf, count);
}

uint64 __sys_close(int fd)
{
    struct file *f = current->ofile[fd];
    if(f == 0)
        return -1;
    current->ofile[fd] = 0;
    fileclose(f);
    return 0;
}

int __sys_mknod(const char *pathname, short major, short minor)
{
    struct inode *ip;
    char path[MAXPATH] = {0};
    if(pathname == 0)
        return -1;
    strncpy(path, pathname, MAXPATH);

    log_begin_op();
    if( (ip = create(path, T_DEVICE, major, minor))  == 0){
        log_end_op();
        return -1;
    }
    
    iunlockput(ip);
    log_end_op();
    
    return 0;
}
