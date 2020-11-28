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
#define T_ERR 4



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



int diskinfo(int fsfd) {        //function that prints the details of the superblock
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

////////////////////////////////////// start errors from here ////////////////////////////////////////////

int err1(int fsfd){
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    int inum = 1;
    struct dinode current_inode;

    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
            perror("lseek");
            exit(1);
        }
        if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
            perror("read");
            exit(1);
        }

    memmove(&current_inode, buf, sizeof(current_inode));
    current_inode.type = T_ERR;

    write(fsfd,&current_inode,sizeof(current_inode));
    
    printf("Changed the type of a inode");

    return 1;
}

int err2(int fsfd){
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    int inum = 5;
    struct dinode current_inode;

    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
            perror("lseek");
            exit(1);
        }
        if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
            perror("read");
            exit(1);
        }

    memmove(&current_inode, buf, sizeof(current_inode));
    current_inode.addrs[0] = 100000;
	current_inode.addrs[11] = 100000;

    write(fsfd,&current_inode,sizeof(current_inode));

    return 1;
}

int err3(int fsfd){
    uint b = 0;
    

    if(lseek(fsfd, 59*BSIZE, 0) != 59 * BSIZE){
        perror("lseek");
        exit(1);
    }
	write(fsfd,&b,1);

    
    return 1;
}

int err4(int fsfd){
    uint b = 10;
	
    if(lseek(fsfd, 59*BSIZE+1, 0) != 59 * BSIZE+1){
        perror("lseek");
        exit(1);
    }
	write(fsfd,&b,1);
	
    return 1;
}

int err5(int fsfd){
	uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
	struct dinode inode1;
    uint buf2;
    rinode(10,&inode1);
    uint bmapbyte;

    int inodeaddr = inode1.addrs[1]/4;

    if(lseek(fsfd, sb.bmapstart*BSIZE + inodeaddr, 0) != sb.bmapstart * BSIZE  + inodeaddr){
        perror("lseek");
        exit(1);
    }

    uint a = 0;

    if (write(fsfd,&a,1) != 1){
        perror("write");
        exit(1);
    }

    printf("Used inode marked free in bitmap");
    return 1;

}

int err6(int fsfd){
	uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
	
    uint bmapbyte;

    if(lseek(fsfd, sb.bmapstart*BSIZE + 100, 0) != sb.bmapstart * BSIZE  + 100){
        perror("lseek");
        exit(1);
    }

    uint a = 255;

    if (write(fsfd,&a,1) != 1){
        perror("write");
        exit(1);
    }

    return 1;

}

int err7(int fsfd) {
  uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
  struct dinode inode5;
  struct dinode inode6;

  rinode(5,&inode5);
  rinode(6,&inode6);

  printf("inode5 address : %d \n", inode5.addrs[0]);
  printf("inode6 address : %d \n", inode6.addrs[0]);

  printf ("making error...");
  
  inode6.addrs[0] = inode5.addrs[0];

  winode(6,&inode6);
  printf("inode6 address now : %d \n", inode6.addrs[0]);
  return 1;
}

int err8(int fsfd) {
  
  struct dinode inode1;
  struct dinode inode2;

  int inum=3;

  //To get two inodes which point to indirect blocks ....
  do {
    rinode(inum, &inode1);
    inum += 1;
  } while (inode1.addrs[NDIRECT] == 0);


  do {
    rinode(inum, &inode2);
    inum += 1;
  } while (inode2.addrs[NDIRECT] == 0);

  uint addr1,addr2;

  lseek(fsfd, inode1.addrs[NDIRECT] * BSIZE + 1*sizeof(uint), SEEK_SET);
  read(fsfd, &addr1, sizeof(uint));

  lseek(fsfd, inode2.addrs[NDIRECT] * BSIZE + 2*sizeof(uint), SEEK_SET);
  read(fsfd, &addr2, sizeof(uint));


  printf("indirect inode1 address : %d \n", addr1);
  printf("indirect inode2 address : %d \n", addr2);

  lseek(fsfd, inode2.addrs[NDIRECT] * BSIZE + 2*sizeof(uint), SEEK_SET);
  write(fsfd, &addr1, sizeof(uint));

  printf("making error....");

  lseek(fsfd, inode2.addrs[NDIRECT] * BSIZE + 2*sizeof(uint), SEEK_SET);
  read(fsfd, &addr2, sizeof(uint));

  printf("indirect inode2 address : %d \n", addr2);

  return 1;
}

int err9(int fsfd){
  uchar buf[BSIZE];
	rsect(SUPERBLOCK,buf);
	memmove(&sb, buf, sizeof(sb));

  int inum;
  struct dinode current_inode;
  for (inum=0; inum<sb.ninodes; inum++){	
  if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	  perror("lseek");
    exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	  perror("read");
    exit(1);
	}
  memmove(&current_inode, buf, sizeof(current_inode));
  if (current_inode.type==0){
    current_inode.type = T_FILE;
    write(fsfd,&current_inode,sizeof(current_inode));
    printf("marked a free inode as used Inode");
    break;}}
  return 1;
}


int err10(int fsfd){
  uchar buf[BSIZE];
	rsect(SUPERBLOCK,buf);
	memmove(&sb, buf, sizeof(sb));

  int inum;
  struct dinode current_inode;
  for (inum=0; inum<sb.ninodes; inum++){	
  if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	  perror("lseek");
    exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	  perror("read");
    exit(1);
	}
  memmove(&current_inode, buf, sizeof(current_inode));
  if (current_inode.type==2){
    current_inode.type=0;
    write(fsfd,&current_inode,sizeof(current_inode));
    printf("marked a used inode as free inode\n");
    break;}}
  return 1;
}

int err11(int fsfd) {
  
  uchar buf[BSIZE];
	rsect(SUPERBLOCK,buf);
	memmove(&sb, buf, sizeof(sb));

  struct dinode inode;

  int inum=3;

  do {
    
    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
          perror("lseek");
          exit(1);
      }
    
    if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
        perror("read");
          exit(1);
    }
    
    memmove(&inode, buf, sizeof(inode));

    inum += 1;

  }while (inode.type != T_FILE);

  if (inum != 3) {
    inum -= 1;
  }
  
  
  printf("no of links of current inode : %d \n", inode.nlink);

  inode.nlink += 1;

  if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
          perror("lseek");
          exit(1);
  }

  write(fsfd,&inode,sizeof(inode));

  if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
          perror("lseek");
          exit(1);
  }
    
  if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
        perror("read");
          exit(1);
  }

  memmove(&inode, buf, sizeof(inode));

  printf("no of links of current inode now : %d \n", inode.nlink);

  return 1;

}

int err12(int fsfd){
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    int inum = 1;
    struct dinode current_inode;

    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
        perror("lseek");
        exit(1);
    }
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	    perror("read");
        exit(1);
	}

    memmove(&current_inode, buf, sizeof(current_inode));
    current_inode.type = T_DIR;
    current_inode.nlink = T_ERR;
    write(fsfd,&current_inode,sizeof(current_inode));

    printf("Changed the type of an inode to directory inode and changed the number of links to greater than 1");

    return 1;
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////


//////Switch Table////////

int switch_table(int err){
    
    if(err == 1){
        err1(fsfd);
    }
    else if(err == 2){
        err2(fsfd);
    }
    else if(err == 3){
        err3(fsfd);
    }
    else if(err == 4){
        err4(fsfd);
    }
    else if(err == 5){
        err5(fsfd);
    }
    else if(err == 6){
        err6(fsfd);
    }
    else if(err == 7){
        err7(fsfd);
    }
    else if(err == 8){
        err8(fsfd);
    }
    else if(err == 9){
        err9(fsfd);
    }
    else if(err == 10){
        err10(fsfd);
    }
    else if(err == 11){
        err11(fsfd);
    }
    else if(err == 12){
        err12(fsfd);
    }
    else{
    printf("ERROR NO. should be in between (1-12)");
    }
    return 0;
}
/////////////////////////

int main(int argc, char* argv[]){

    if(argc != 3){
        printf("Usage : ./error.o (ERROR No.{1-12}) (File System .img file)\n");
        return 0;
    }    

    // Read the File System image with file discriptor fsfd
    if ((fsfd = open (argv[2],O_RDWR, 0666)) == -1) { /* open/validate file */
        perror ("Error opening file :\n");
        exit (EXIT_FAILURE);
    }

    // diskinfo(fsfd);
    switch_table(atoi(argv[1]));
    return 0;
}
