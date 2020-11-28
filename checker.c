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

/////////////////////////// Checker Functions from here ////////////////////////////////

// Print the error statements in the functions itself

int check1(int fsfd){
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
    int i;   
    struct dinode inode1;
    for (i = 1;i<=NINODE;i++){
        rinode(i,&inode1);
        if((inode1.type != 0) && (inode1.type != 1) && (inode1.type != 2) && (inode1.type != 3)){
            printf("ERROR: bad inode\n");
            return 1;
        }
    }
    return 0; // return 1 if error is detected
}

int check2(int fsfd){
    int err = 0;
    
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    int i;   
    struct dinode inode1;
    int data_block_start = sb.size-sb.nblocks;
    for (i = 1;i<=NINODE;i++){
        rinode(i,&inode1);
        for(int j = 0;j<NDIRECT;j++){
            if(inode1.addrs[j] != 0){
                if(inode1.addrs[j] < data_block_start || inode1.addrs[j] > sb.size){
                    printf("ERROR: bad direct address in inode : %d\n",i);
                    err = 1;
                }
            }
        }
        if(inode1.addrs[NDIRECT] != 0){
            if (lseek(fsfd, inode1.addrs[NDIRECT] * BSIZE , 0)!= inode1.addrs[NDIRECT] * BSIZE){
				perror("lseek");
				exit(1);
			}
            uint buf2;
            int k;
			for(k=0; k<NINDIRECT; k++){														//loop through indirect ptrs
				if (read(fsfd, &buf2, sizeof(uint))!= sizeof(uint)){						//read the next addr
					perror("read");
					exit(1);
				}
				if (buf2!=0){													//if block is in use
                    if ((buf2<data_block_start) || (buf2 > sb.size)){			//if block is out of bounds
					    printf("ERROR: bad indirect address in inode : %d\n",i);
                        err = 1;
                    }
				}
			}

        }
    }
    if(err == 1){
        return 1;
    }
    return 0; // return 1 if error is detected
}

int check3(int fsfd){

    int err = 0;
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
    struct dinode inode1;
    rinode(1,&inode1);
    if(inode1.type != T_DIR){
        printf("ERROR: root directory does not exist\n");
        err = 1;
        return 1;
    }

    char *p = NULL; 
    if ((p = mmap (NULL, FSSIZE*BSIZE, PROT_READ, MAP_PRIVATE, fsfd, 0)) == (void *) -1) {
        perror ("mmap failed");
        exit (EXIT_FAILURE);
    }

    if(inode1.addrs[0] != 0){
        if(lseek(fsfd, inode1.addrs[0]*BSIZE, 0) != inode1.addrs[0] * BSIZE){
            perror("lseek");
            exit(1);
        }
        char buf2[BSIZE];
        if(read(fsfd,&buf2,BSIZE) != BSIZE){
            perror("read");
            exit(1);
        }

        struct dirent* root = (struct dirent *)(p+ inode1.addrs[0]*BSIZE);
        if((root+1)->inum != root->inum){
            printf("ERROR: root directory does not exist\n");
            err = 1;
            return 1;
        }
        // for(int k = 0;k<20;k++){
        //     printf("%c\n",buf2[k]);
        // }
    }
    else{
        printf("ERROR: root directory does not exist\n");
        err = 1;
        return 1;
    }
    return 0; // return 1 if error is detected
}

int check4(int fsfd){

    return 0; // return 1 if error is detected
}

int check5(int fsfd){

    return 0; // return 1 if error is detected
}

int check6(int fsfd){

    return 0; // return 1 if error is detected
}

int check7(int fsfd){
    int err = 0;

    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    // initializing array address with zeores
    uint address[sb.size];
    for(int k=0;k<sb.size;k++){
        address[k]=0;
    }

    int i;   
    struct dinode inode1;
    for (i = 1;i<=NINODE;i++){
        rinode(i,&inode1);  // loop to read all inodes
        for(int j = 0;j<NDIRECT;j++){
            if(address[inode1.addrs[j]] == 1){
                printf("ERROR: direct address used more than once\n");
                err = 1;
                return 1;
            }
            else if(inode1.addrs[j] != 0){
                address[inode1.addrs[j]] = 1;   //assigning the value one as an indication of being used
            }
        }
    }
    if(err == 1){
        return 1;
    }
    return 0; // return 1 if error is detected
}


int check8(int fsfd){
    int err = 0;

    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    // initializing array address with zeores
    uint address[sb.size];
    for(int k=0;k<sb.size;k++){
        address[k]=0;
    }

    int i;
    uint addr; 
    struct dinode inode1;
    for (i = 1;i<=NINODE;i++){
        rinode(i,&inode1);  // loop to read all inodes
        if(inode1.addrs[NDIRECT] != 0){
            for(int j=0; j<NINDIRECT; j++){
                lseek(fsfd, inode1.addrs[NDIRECT] * BSIZE + j*sizeof(uint), SEEK_SET);      //loop through indirect ptrs
                read(fsfd, &addr, sizeof(uint));                                            //read the next addr

                if(addr == 0){
                    continue;
                }

                if(address[addr] == 1){
                    printf("ERROR: indirect address used more than once\n");
                    err = 1;
                    return 1;
                }
                address[addr] = 1;   //assigning the value one as an indication of being used
            }
        }

    }
    if(err == 1){
        return 1;
    }
    return 0; // return 1 if error is detected
}

int check9(int fsfd){
    uchar buf[BSIZE];
	rsect(SUPERBLOCK,buf);
	memmove(&sb, buf, sizeof(sb));
    int indir[sb.ninodes];
    for (int in=0; in<sb.ninodes; in++){indir[in]=0;}
    struct dinode current_inode;
    for (int inum=0; inum<sb.ninodes; inum++){	
    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	    perror("read");
        exit(1);
	}
    memmove(&current_inode, buf, sizeof(current_inode));
    if (current_inode.type==T_DIR){
		for(int x=0; x<NDIRECT; x++){
			if(current_inode.addrs[x]!=0) {
			if (lseek(fsfd,current_inode.addrs[x]*BSIZE, SEEK_SET) != current_inode.addrs[x]*BSIZE){ 
				perror("lseek");
				exit(1);}
			struct dirent buf;
			for(int de=0; de<(BSIZE/sizeof(struct dirent)); de++){					//loop thorugh all directory entries
				read(fsfd, &buf, sizeof(struct dirent)); 
				if(buf.inum!=inum){
					indir[buf.inum]=1;//making all directory entries 1
				}}}}
		uint directory_address;
		if (current_inode.addrs[NDIRECT]!=0){		//if there is an indirect pointers
			for(int y=0; y<NINDIRECT; y++){
				if (lseek(fsfd, current_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint), SEEK_SET) != current_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint)){
					perror("lseek");
					exit(1);
				}
				if (read(fsfd, &directory_address, sizeof(uint)) != sizeof(uint)){
					perror("read");
					exit(1);	
				}
				if(directory_address!=0) {
				if (lseek(fsfd,directory_address*BSIZE, SEEK_SET) != directory_address*BSIZE){
					perror("lseek");
					exit(1);}
				struct dirent buf;
				for(int i=0;i<(BSIZE/sizeof(struct dirent));i++){ //loop thorugh all dir entries
					read(fsfd, &buf, sizeof(struct dirent)); 
				if(buf.inum!=inum){
					indir[buf.inum]=1;//making all directory entries 1
					}}}}}}}
    for (int inum=0; inum<sb.ninodes; inum++){	
    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	    perror("read");
        exit(1);
	}
    memmove(&current_inode, buf, sizeof(current_inode));
    if (current_inode.type!=0){
        if (indir[inum]==0 && inum!=1){
            printf("ERROR: inode marked use but not found in a directory.\n");
            return(1);
        }}}

    return 0; 
}


int check10(int fsfd){

    uchar buf[BSIZE];
	rsect(SUPERBLOCK,buf);
	memmove(&sb, buf, sizeof(sb));
    int indir[sb.ninodes];
    for (int in=0; in<sb.ninodes; in++){indir[in]=0;}
    struct dinode current_inode;
    for (int inum=0; inum<sb.ninodes; inum++){	
    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	    perror("read");
        exit(1);
	}
    memmove(&current_inode, buf, sizeof(current_inode));
    if (current_inode.type==T_DIR){
		for(int x=0; x<NDIRECT; x++){
			if(current_inode.addrs[x]!=0) {
			if (lseek(fsfd,current_inode.addrs[x]*BSIZE, SEEK_SET) != current_inode.addrs[x]*BSIZE){ 
				perror("lseek");
				exit(1);}
			struct dirent buf;
			for(int de=0; de<(BSIZE/sizeof(struct dirent)); de++){					//loop thorugh all directory entries
				read(fsfd, &buf, sizeof(struct dirent)); 
				if(buf.inum!=inum){
					indir[buf.inum]=1;//making all directory entries 1
				}}}}
		uint directory_address;
		if (current_inode.addrs[NDIRECT]!=0){		//if there is an indirect pointers
			for(int y=0; y<NINDIRECT; y++){
				if (lseek(fsfd, current_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint), SEEK_SET) != current_inode.addrs[NDIRECT] * BSIZE + y*sizeof(uint)){
					perror("lseek");
					exit(1);
				}
				if (read(fsfd, &directory_address, sizeof(uint)) != sizeof(uint)){
					perror("read");
					exit(1);	
				}
				if(directory_address!=0) {
				if (lseek(fsfd,directory_address*BSIZE, SEEK_SET) != directory_address*BSIZE){
					perror("lseek");
					exit(1);}
				struct dirent buf;
				for(int i=0;i<(BSIZE/sizeof(struct dirent));i++){ //loop thorugh all dir entries
					read(fsfd, &buf, sizeof(struct dirent)); 
				if(buf.inum!=inum){
					indir[buf.inum]=1;//making all directory entries 1
					}}}}}}}
    for (int inum=0; inum<sb.ninodes; inum++){	
    if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	}
	if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
	    perror("read");
        exit(1);
	}
    memmove(&current_inode, buf, sizeof(current_inode));
    if (indir[inum]==1){
        if (current_inode.type==0 && inum!=0){
            printf("ERROR: inode referred to in directory but marked free.\n");
            return(1);
        }}
        }

    return 0; 
}


int check11(int fsfd){

    return 0; // return 1 if error is detected
}

int check12(int fsfd){

    return 0; // return 1 if error is detected
}

////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////// Check Image ///////////////////////////////////////////

int check_fsimage(int fsfd){
    int total;
    total = check1(fsfd)+check2(fsfd)+check3(fsfd)+check4(fsfd)+check5(fsfd)+check6(fsfd)+check7(fsfd)+check8(fsfd)+check9(fsfd)+check10(fsfd)+check11(fsfd)+check12(fsfd); 
    if(total > 0){
        return 1;
    }
    else{
        printf("There are no errors in the file system image\n");
    }
}

////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]){
	if(argc != 2){
        printf("Usage : ./checker.o (File System .img file)\n");
        return 0;
    }    

    // Read the File System image with file discriptor fsfd
    if ((fsfd = open (argv[1],O_RDWR, 0666)) == -1) { /* open/validate file */
        perror ("Error opening file :\n");
        exit (EXIT_FAILURE);
    }
    //diskinfo(fsfd);

    check_fsimage(fsfd);

    return 0;
}
