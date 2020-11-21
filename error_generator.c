#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"
#define SUPERBLOCK 1




int fsfd;
struct superblock sb;


ushort
xshort(ushort x)
{
  ushort y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  return y;
}

uint
xint(uint x)
{
  uint y;
  uchar *a = (uchar*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

void
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(read(fsfd, buf, BSIZE) != BSIZE){
    perror("read");
    exit(1);
  }
}

void
rinode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *ip = *dip;
}

void
wsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * BSIZE, 0) != sec * BSIZE){
    perror("lseek");
    exit(1);
  }
  if(write(fsfd, buf, BSIZE) != BSIZE){
    perror("write");
    exit(1);
  }
}

void
winode(uint inum, struct dinode *ip)
{
  char buf[BSIZE];
  uint bn;
  struct dinode *dip;

  bn = IBLOCK(inum, sb);
  rsect(bn, buf);
  dip = ((struct dinode*)buf) + (inum % IPB);
  *dip = *ip;
  wsect(bn, buf);
}



int diskinfo(int fsfd) {

    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    printf("bitmap start: %d\n",sb.bmapstart);
    printf("inode start: %d\n",sb.inodestart);
    printf("log start : %d\n",sb.logstart);
    printf("nblocks : %d\n",sb.nblocks);
    printf("ninodes : %d\n",sb.ninodes);
    printf("nlog : %d\n",sb.nlog);
    printf("size : %d\n",sb.size);
    return 0;
}



int main(int argc, char* argv[]){

    if(argc != 3){
        printf("Usage : ./error.o (ERROR No.{1-12}) (File System .img file)\n");
        return 0;
    }    

    // Read the File System image with file discriptor fsfd
    if ((fsfd = open (argv[1],O_RDWR, 0666)) == -1) { /* open/validate file */
        perror ("Error opening file :\n");
        exit (EXIT_FAILURE);
    }

    diskinfo(fsfd);

    return 0;
}
