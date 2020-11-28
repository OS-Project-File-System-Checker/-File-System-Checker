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
    int err = 0;
    
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));
    
    struct dinode inode1;

    char *p = NULL; 
    if ((p = mmap (NULL, FSSIZE*BSIZE, PROT_READ, MAP_PRIVATE, fsfd, 0)) == (void *) -1) {
        perror ("mmap failed");
        exit (EXIT_FAILURE);
    }

    for (int i = 1;i<=NINODE;i++){
        rinode(i,&inode1);
        if (inode1.type == 1){
            struct dirent* dir = (struct dirent *)(p+ inode1.addrs[0]*BSIZE);
            if ((strcmp(dir->name, ".")) != 0) {
                printf("ERROR: directory not properly formatted : \".\"\n");          
                err = 1;
            }
            if ((strcmp((dir + 1)->name, "..")) != 0) {
                printf("ERROR: directory not properly formatted : \".\"\n");
                err = 1;
            }
            if (dir->inum != i){
                
                printf("ERROR: directory not properly formatted : \".\" -> inode: \"%d\"\n",i);
                err = 1;
            }
        }
    }
    
    if(err ==1){
        return 1;   
    }
    return 0; // return 1 if error is detected
}

int check5(int fsfd){int err=0;
    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

	struct dinode inode1;
    
    for (int i = 0;i<=NINODE;i++){
	    uint buf2;
        uint buf;
        rinode(i,&inode1);  
        for(int j = 0;j<NDIRECT;j++)
        {
            if(inode1.addrs[0]!=0)
            {
                if (lseek(fsfd, sb.bmapstart*BSIZE + inode1.addrs[j]/8,SEEK_SET) != sb.bmapstart*BSIZE + inode1.addrs[j]/8 ){
				    perror("lseek");
				    exit(1);
			    }
			    if (read(fsfd, &buf2, 1) != 1){	
				    perror("read");
				    exit(1);
			    }
                int shift_amt=inode1.addrs[j]%8;
			    buf2=buf2 >> shift_amt;
			    buf2=buf2%2;
                if(buf2==0) {
                    printf("ERROR: address used by inode but marked free in bitmap\n");
                    err=1; 
                    return 1;  
                }      
            }
        }

        if(inode1.addrs[NDIRECT] != 0){											
		int x;
		for(x=0; x<NINDIRECT; x++){														
		    if (lseek(fsfd, inode1.addrs[NDIRECT] * BSIZE + x*sizeof(uint), SEEK_SET) != inode1.addrs[NDIRECT] * BSIZE + x*sizeof(uint)){
		        perror("lseek");
			    exit(1);
            }
		    if (read(fsfd, &buf, sizeof(uint)) != sizeof(uint)){														//read the next entry
			    perror("read");
			    exit(1);
		    }
		    if (buf!=0){		
			    if (lseek(fsfd, sb.bmapstart*BSIZE + buf/8,SEEK_SET) != sb.bmapstart*BSIZE + buf/8){ 
			    perror("lseek");
			    exit(1);
		    }
		    if (read(fsfd, &buf2, 1) != 1){	
			    perror("read");
			    exit(1);
		    }
		    int shift_amt=inode1.addrs[i]%8;
		    buf2=buf2 >> shift_amt;
		    buf2=buf2%2;
		    if(buf2==0) {
                printf("ERROR: address used by inode but marked free in bitmap\n");
                return 1;
            }
		}
	    }	
        }
    }
    return 0; // return 1 if error is detected
}

int check6(int fsfd){
    int err = 0;

    uchar buf[BSIZE];
    rsect(SUPERBLOCK,buf);
    memmove(&sb, buf, sizeof(sb));

    uint add1[sb.size];             //add of inodes
    for(int k=0;k<sb.size;k++){
        if(k < (sb.size - sb.nblocks)){
            add1[k] = 1;
        }
        else
        {
            add1[k]=0;
        }
    }

    uint add2[sb.size];             //add of bitmap
    for(int k=0;k<sb.size;k++){
        add2[k]=0;
    }

    int i;   
    uint addr; 
    struct dinode inode1;
    for (i = 1;i<=NINODE;i++){
        rinode(i,&inode1);  // loop to read all inodes
        if(inode1.type != 0){
            for(int j = 0;j<=NDIRECT;j++){
                if(inode1.addrs[j] != 0){
                    add1[inode1.addrs[j]] =1;
                }
            }
            if(inode1.addrs[NDIRECT] != 0){
                for(int j=0; j<NINDIRECT; j++){
                    lseek(fsfd, inode1.addrs[NDIRECT] * BSIZE + j*sizeof(uint), SEEK_SET);      //loop through indirect ptrs
                    read(fsfd, &addr, sizeof(uint));                                            //read the next addr
                    if(addr != 0){
                        add1[addr] = 1;
                    }
                }
            }
        }
    }

    if(lseek(fsfd, sb.bmapstart*BSIZE, 0) != sb.bmapstart * BSIZE){
        perror("lseek");
        exit(1);
    }
    uint buf3;
    for( i = 0;i<1000/8;i++){
    if (read(fsfd, &buf3, 1) != 1){	//read byte containing bit we want
        perror("read");
        exit(1);    
      }
      for(int j = 0;j<8;j++){
        if ((buf3 >> 2)%2 == 1){
            add2[i*8 + j] = 1;
        }
      }
    }

    for(i = 0;i<sb.size;i++){
        if ((add2[i] == 1) && (add1[i] == 0)){
            
            err = 1;
        }
    }

    if (err == 1){
        printf("ERROR: bitmap marks block in use but it is not in use\n");
        return 1;
    }
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

int check_direct_refs(struct dinode current_inode, int count, int inum) {   // Function to calculate the number of directories in the direct blocks of an inode  
									    // which refer to a file
    struct dirent dir;            

    for(int i = 0; i <NDIRECT; i++)     // Iterating through all the direct blocks 
    {
        if(current_inode.addrs[i] != 0)
        {

            for(int j=0;j<BSIZE/sizeof(struct dirent);j++)  // Iterating through all the directory structures in a direct block
            {
                
                if (lseek(fsfd, current_inode.addrs[i] * BSIZE + j*sizeof(struct dirent), SEEK_SET) != current_inode.addrs[i] * BSIZE + j*sizeof(struct dirent)){  //move to correct location
                    perror("lseek");
                    exit(1);
                }

                if(read(fsfd, &dir, sizeof(struct dirent))!=sizeof(struct dirent)){	//read directory entry info into buffer
                    perror("read");
                    exit(1);
                }
    
                if(dir.inum == inum)    // If there is a matching inode number, increment the no. of direct refs
                {                       // of that direct block by 1 and proceed to next direct block
                    count += 1;
                    break;
                }            
            }
            
        }
        
    }

    return count;    

}

int check_indirect_refs(struct dinode current_inode, int count, int inum) {	// Function to calculate the number of directories in the indirect blocks of an inode  
									    	// which refer to a file
    struct dirent dir;

    uint current_address;

    if(current_inode.addrs[NDIRECT] != 0)   // If there is a reference to indirect block, proceed
    {
        for(int i = 0; i <NINDIRECT; i++)   // Iterating through addresses in the indirect block
        {
            if (lseek(fsfd, current_inode.addrs[NDIRECT] * BSIZE + i * sizeof(uint), SEEK_SET) != current_inode.addrs[NDIRECT] * BSIZE + i * sizeof(uint)){  //move to correct location
                perror("lseek");
                exit(1);
            }

            read(fsfd, &current_address, sizeof(uint));
    
            if(read(fsfd, &current_address, sizeof(uint))!=sizeof(uint)){	//read address of the indirect block into buffer
                perror("read");
                exit(1);
            }
            
            if( current_address != 0)   
            {
                for(int j=0;j<BSIZE/sizeof(struct dirent);j++)  // Iterating through all the directory structures in the indirect block
                {                    
                    if (lseek(fsfd, current_inode.addrs[i] * BSIZE + j*sizeof(struct dirent), SEEK_SET) != current_inode.addrs[i] * BSIZE + j*sizeof(struct dirent)){  //move to correct location
                        perror("lseek");
                        exit(1);
                    }

                    if(read(fsfd, &dir, sizeof(struct dirent))!=sizeof(struct dirent)){	//read directory entry info into buffer
                        perror("read");
                        exit(1);
                    }
        
                    if(dir.inum == inum)    // If there is a matching inode number, increment the no. of refs
                    {                       // of that indirect block by 1 and proceed to next indirect block
                        count += 1;
                        break;
                    }            
                }
                
            }
            
        }
    }

    return count;
}

int total_num_ref(uint current_inum) //
{
    int count = 0;
    struct dinode current_inode;

    for(int inum = 0; inum < sb.ninodes; inum++)
    {       
        if (inum != current_inum) {
        
            if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
                perror("lseek");
                exit(1);
            }
        
            if(read(fsfd, &current_inode, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
                perror("read");
                exit(1);
            }
            
            if (current_inode.type == T_DIR) {

                // Updating the number of refs by checking the direct and indirect blocks of the current inode

                count = check_direct_refs(current_inode, count, current_inum);    

                count = check_indirect_refs(current_inode, count, current_inum);
            }        
        }        
    }

    return count;
}

int check11(int fsfd){

    struct dinode current_inode;
    char buf[sizeof(struct dinode)];
    
    for (int inum=0;inum<(int)sb.ninodes;inum++)
    {
        
        if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	    }
	
        if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
            perror("read");
            exit(1);
        }
        
        memmove(&current_inode, buf, sizeof(current_inode));

        if(current_inode.type == T_FILE && current_inode.nlink != total_num_ref(inum))
        {

            printf("ERROR: bad reference count for file.");
            close(fsfd);
            return 1;
        
        }

    }

    return 0; // return 1 if error is detected
}


int check12(int fsfd){

    struct dinode current_inode;
    char buf[sizeof(struct dinode)];

    for (int inum=0;inum<(int)sb.ninodes;inum++)
    {
        
        if (lseek(fsfd, sb.inodestart * BSIZE + inum * sizeof(struct dinode), SEEK_SET) != sb.inodestart * BSIZE + inum * sizeof(struct dinode)){  //move to correct location
	    perror("lseek");
        exit(1);
	    }
	
        if(read(fsfd, buf, sizeof(struct dinode))!=sizeof(struct dinode)){	//read inode info into buffer
            perror("read");
            exit(1);
        }
        
        memmove(&current_inode, buf, sizeof(current_inode));
        
        if(current_inode.type == T_DIR && current_inode.nlink > 1)
        {
            printf("ERROR: directory appears more than once in file system.\n");
            close(fsfd);
            return 1;
        }
    }

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
