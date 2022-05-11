#ifndef __FS_H__
#define __FS_H__
#include "types.h"

// On-disk file system format.
// Both the kernel and user programs use this header file.


#define ROOTINO  1   // root i-number
#define BSIZE 1024  // block size

// Disk layout:
// [ boot block | super block | log | inode blocks |
//                                          free bit map | data blocks]
//
// mkfs computes the super block and builds an initial file system. The
// super block describes the disk layout:
struct superblock {
  uint magic;        // Must be FSMAGIC
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block
};

#define FSMAGIC 0x10203040

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEVICE only)
  short minor;          // Minor device number (T_DEVICE only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block of free map containing bit for block b
#define BBLOCK(b, sb) ((b)/BPB + sb.bmapstart)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

#define min(a, b) ((a) < (b) ? (a) : (b))

void fsinit(int dev);

struct inode;
void ilock(struct inode *ip);
void iunlock(struct inode *ip);
struct inode *ialloc(int dev, short type);
int readi(struct inode *ip, uint64 dst, uint off, uint n);
int writei(struct inode *ip, uint64 src, uint off, uint n);
int dirlink(struct inode *dp, char *name, uint inum);
struct inode *namei(char *path);
struct inode* nameiparent(char *path, char *name);
struct inode *dirlookup(struct inode *dp, char *name, uint *poff);
void iunlockput(struct inode *ip);
void iupdate(struct inode *ip);
void iput(struct inode *ip);
void itrunc(struct inode *ip);

#endif
