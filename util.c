/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/file.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE];
extern MINODE *root;
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev;
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

// extern char *linkParameters[64];

int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   int n = read(dev, buf, BLKSIZE);
   if(n < 0)
      printf("get_block [%d %d] error", dev, blk);
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   int n = write(dev, buf, BLKSIZE);
   if(n != BLKSIZE)
      printf("put_block [%d %d] error", dev, blk);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

// return minode pointer to loaded INODE
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){
       mip->refCount++;
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip;
    }
  }
    
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + iblk;
       offset = (ino-1) % 8;

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);
       ip = (INODE *)buf + offset;
       // copy INODE to mp->INODE
       mip->INODE = *ip;
       return mip;
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)
{
  INODE *ip;
  int i, block, offset;
  char buf[BLKSIZE];
  if (mip==0) return;
  mip->refCount--; // dec refCount by 1
  if (mip->refCount > 0) return; // still has user
  if (mip->dirty == 0) return; // no need to write back
  // write INODE back to disk
  block = (mip->ino - 1) / 8 + iblk;
  offset = (mip->ino - 1) % 8;
  // get block containing this inode
  get_block(mip->dev, block, buf);
  ip = (INODE *)buf + offset; // ip points at INODE
  *ip = mip->INODE; // copy INODE to inode in block
  put_block(mip->dev, block, buf); // write back to disk
  mip->dirty = 0;
} 

int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;  // inode pointer

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     printf("%4d  %4d  %4d    %s\n", 
           dp->inode, dp->rec_len, dp->name_len, dp->name);
     if (strcmp(temp, name)==0){
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0;
}

// Returns the block number of the inode.
int getino(char *pathname)
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);

  // The pathname = / return 2 i.e. root.
  if (strcmp(pathname, "/")==0)
      return 2;
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname);

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
      if (!S_ISDIR(mip->INODE.i_mode)) 
      { // check DIR type
        printf("%s is not a directory\n", name[i]);
        iput(mip);
        return 0;
      }
      ino = search(mip, name[i]);
      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }
      iput(mip);
      mip = iget(dev, ino);
   }

   iput(mip);
   //printf("\njddThe ino returned from search function: %d",ino);
   return ino;
}

int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  char buf[1024];
  INODE inode = parent->INODE;
  char *current;
  DIR *directory;
  u32 block;

  // If it is a directory
  if (inode.i_mode == 16877) 
  {
    // Considering only 12 blocks.
    for (int i = 0; i < 12; i++) 
    {
      // Storing the block number of the i-th block.
      block = inode.i_block[i];

      // If the block in not empty
      if (block != 0) 
      {
        // Getting the block.
        get_block(parent->dev, block, buf);

        current = buf; // copying buf into current
        directory = (DIR *)buf;

        // Parsing the directory.
        while (current < &buf[BLKSIZE]) 
        {
          // If the directoy Inode number is the ino of the directory to be removed:
          if (directory->inode == myino) 
          {
            // coppying name if correct one we want
            strcpy(myname, directory->name);
            printf("\nFound.");
            return 0;
          }
          // add current
          current += directory->rec_len;
          directory = (DIR *)current;
        }
      }
      else 
      {
        printf("Block is 0\n");
      }
    }
    return 0;
  }

  printf("No directory found\n");
  return 0;
}

int tst_bit(char *buf, int bit)
{
  return buf[bit/8] & (1 << (bit % 8));
}

int set_bit(char *buf, int bit)
{
  return buf[bit/8] |= (1 << (bit % 8));
}

// This function gets the super block and the GD and reduces the free inode count.
// Super block contains the info about the whole file system and the GD contains info about a group of blocks.
// Since we're dealing with only 1024 blocks hence, we can consider that there will be only 1 block to deal with.
void decFreeInodes(int dev)
{
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count--;
  put_block(dev, 2, buf);
}

// This function gets the super block and the GD and reduces the free inode count.
// Super block contains the info about the whole file system and the GD contains info about a group of blocks.
// Since we're dealing with only 1024 blocks hence, we can consider that there will be only 1 block to deal with.
void decFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count--;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count--;
  put_block(dev, 2, buf);
}

// This function gets the super block and the GD and reduces the free inode count.
// Super block contains the info about the whole file system and the GD contains info about a group of blocks.
// Since we're dealing with only 1024 blocks hence, we can consider that there will be only 1 block to deal with.
void incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

// This function gets the super block and the GD and reduces the free inode count.
// Super block contains the info about the whole file system and the GD contains info about a group of blocks.
// Since we're dealing with only 1024 blocks hence, we can consider that there will be only 1 block to deal with.
void incFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  bzero(buf, BLKSIZE);
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}


// allocate an inode number from inode_bitmap
int ialloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        decFreeInodes(dev);
        return i+1;
    }
  }
  return 0;
}

int balloc(int dev)
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, bmap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; bmap from 1
        decFreeBlocks(dev);
        return i+1;
    }
  }
  return 0;
}


int findino(MINODE *mip, int *myino, int *parentino)
{
	/*
	For a DIR Minode, extract the inumbers of . and ..
	Read in 0th data block. The inumbers are in the first two dir entries.
	*/
	INODE *ip;
	char buf[1024];
	char *cp;
	DIR *dp;

	//check if exists
	if(!mip)
	{
		printf("ERROR: ino does not exist!\n");
		return 1;
	}

	//point ip to minode's inode
	ip = &mip->INODE;

	//check if directory
	if(!S_ISDIR(ip->i_mode))
	{
		printf("ERROR: ino not a directory\n");
		return 1;
	}

	get_block(dev, ip->i_block[0], buf);
	dp = (DIR*)buf;
	cp = buf;

	//.
	*myino = dp->inode;

	cp += dp->rec_len;
	dp = (DIR*)cp;

	//..
	*parentino = dp->inode;

	return 0;
}

int clr_bit(char *buf, int bit)
{
  buf[bit/8] &= ~(1<<(bit%8));
}

//deallocate ino
int idalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  get_block(dev, imap, buf);//get i to buf
  clr_bit(buf, ino-1);//clr
  put_block(dev, imap, buf);
  incFreeInodes(dev);
}

int bdalloc(int dev, int ino)
{
  int i;
  char buf[BLKSIZE];
  get_block(dev, bmap, buf);  
  clr_bit(buf, ino-1);
  put_block(dev, bmap, buf);
  incFreeBlocks(dev);
}

// void tokenizeLink(char *pathname)
// {
//   char temp[256];
//   int i;
//   int n = 0;
//   char *s;
//   printf("tokenize %s\n", pathname);

//   strcpy(temp, pathname);   // tokens are in global gpath[ ]

//   s = strtok(gpath, " ");
//   while(s){
//     linkParameters[n] = s;
//     n++;
//     s = strtok(0, " ");
//   }
//   linkParameters[n] = 0;
  
//   for (i= 0; i<n; i++)
//     printf("%s  ", linkParameters[i]);
//   printf("\n");
// }