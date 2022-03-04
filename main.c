/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"
#include"util.c"
#include"cd_ls_pwd.c"
#include"mkdir_creat.c"
#include"rmdir.c"
#include"link_unlink.c"
#include"symlink.c"
#include"open_close_lseek.c"
#include"read_cat.c"
#include "write_cp.c"

extern MINODE *iget();
int quit();

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk, inode_start;
char line[128], cmd[32], pathname[128], oldFile[128], newFile[128];
char disk[128]; 
// Level 1: mydisk
// Level 2: disk2

int init()
{
  int i, j;
  MINODE *mip;
  root = 0;
  PROC   *p;

  printf("init()\n");

  // // Initializing the data structures.
  // proc[0] = malloc(sizeof(PROC));
	// proc[1] = malloc(sizeof(PROC));
	// running = malloc(sizeof(PROC));

  for (i=0; i<NMINODE; i++)
  {
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++)
  {
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    for(int j = 0; j<NFD; j++)
    {
      p->fd[j] = 0;    // No open file.
    }
    
  }

  for(int i = 0; i<64; i++)
  {
    oft->refCount = 0;  // FREE
  }
}

// load root INODE and set root pointer to it
int mount_root()
{
  printf("\nmount_root()\n");
  printf("\nchecking EXT2 FS ....");

  if ((dev = open(disk, O_RDWR)) < 0){
    printf("\nopen %s failed\n", disk);
    exit(1);
  }

  /********** read super block  ****************/
  char superBuf[1024];
  get_block(dev, 1, superBuf);
  SUPER *sp = (SUPER*)superBuf;

   /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("\nmagic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }
  
  nblocks = sp->s_blocks_count;     // Number of blocks in the file system.
  ninodes = sp->s_inodes_count;     // Number of inodes in the file system.

  GD group;
  char groupBuf[1024];
  get_block(dev, 2, groupBuf);
  GD *gd = (GD*)groupBuf;
  bmap = gd->bg_block_bitmap;       // 32bit block id of the first block of the "block bitmap" for the group represented. 
  imap = gd->bg_inode_bitmap;       // 32bit block id of the first block of the "inode bitmap" for the group represented.
  iblk = gd->bg_inode_table;        // 32bit block id of the first block of the "inode table" for the group represented.
  printf("\nbmap=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  root = iget(dev, 2);
  proc[0].cwd = iget(dev, 2);
	proc[1].cwd = iget(dev, 2);

  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = iget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

}



int main(int argc, char *argv[ ])
{
  if(argc != 2)
  {
    printf("\nDisk not entered.\n");
    return 0;
  }
  else
  {
    strcpy(disk,argv[1]);
  }
  int ino;
  init();  
  mount_root();
  
  // WRTIE code here to create P1 as a USER process
  
  while(1){
    printf("\ninput command : [ls|cd|pwd|mkdir|creat|rmdir|link|unlink|symlink|readlink|open|close|pfd|read|write|cat|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;

    int count = 0;
    for(int i = 0; i<strlen(line); i++)
    {
      if(line[i] == ' ')
        count++;
    }
    if(count == 1)
      sscanf(line, "%s %s", cmd, pathname);
    else
      sscanf(line, "%s %s %s", cmd, oldFile, newFile);
    printf("cmd=%s pathname=%s\n", cmd, pathname);

    if(pathname[strlen(pathname)-1] == '/')
      pathname[strlen(pathname)-1] = '\0';
  
    if (strcmp(cmd, "ls")==0)
       ls();
    else if (strcmp(cmd, "cd")==0)
       cd();
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "pfd")==0)
       pfd();

    else if(strcmp(cmd, "lseek") == 0)
    {
      mylseek(atoi(oldFile), atoi(newFile));
    }
    else if(strcmp(cmd, "close") == 0)
    {
      closeFile(atoi(pathname));
    }
    else if (strcmp(cmd, "mkdir")==0)
    {
      if(my_mkdir(pathname) == 0)
      {
        printf("\nmkdir failed.");
      }
      else
      {
        printf("\nmkdir successful.");
      }
    }
    else if (strcmp(cmd, "creat")==0)
    {
      if(my_creat(pathname) == 0)
      {
        printf("\ncreat failed.");
      }
      else
      {
        printf("\ncreat successful.");
      }
    }
    else if (strcmp(cmd, "rmdir")==0)
    {
      if(my_rmdir(pathname) != 1)
      {
        printf("\nrmdir failed.");
      }
      else
      {
        printf("\nrmdir successful.");
      }
    }
    else if (strcmp(cmd, "link")==0)
    {
      if(my_link(oldFile,newFile) != 1)
      {
        printf("\nlink failed.");
      }
      else
      {
        printf("\nlink successful.");
      }
    }
       
    else if (strcmp(cmd, "unlink")==0)
    {
      if(my_unlink(pathname) != 1)
      {
        printf("\nunlink failed.");
      }
      else
      {
        printf("\nunlink successful.");
      }
    }

    else if (strcmp(cmd, "symlink")==0)
    {
      if(my_symlink(oldFile,newFile) != 1)
      {
        printf("\nsymlink failed.");
      }
      else
      {
        printf("\nsymlink successful.");
      }
    }

    else if (strcmp(cmd, "open")==0)
    {
      char filepath[124], mode[8];
      strcpy(filepath,oldFile);
      strcpy(mode,newFile);
      if(my_open(filepath,mode) == -1)
      {
        printf("\nopen failed.");
      }
      else
      {
        printf("\nopen successful.");
      }
    }
    else if (strcmp(cmd, "read")==0)
    {
      char fd[8], bytes[8];
      strcpy(fd,oldFile);
      strcpy(bytes,newFile);
      if(readfile(atoi(fd), atoi(bytes)) == -1)
      {
        printf("\read failed.");
      }
      else
      {
        printf("\read successful.");
      }
    }
    else if (strcmp(cmd, "cat")==0)
    {
      if(myCat(pathname) == -1)
      {
        printf("\ncat failed.");
      }
      else
      {
        printf("\ncat successful.");
      }
    }

    else if (strcmp(cmd, "write")==0)
    {
      if(writefile() == -1)
      {
        printf("\nwrite failed.");
      }
      else
      {
        printf("\nwrite successful.");
      }
    }

    else if (strcmp(cmd, "quit")==0)
       quit();
    memset(line, 0, sizeof(line));
    memset(cmd, 0, 64);
	  memset(pathname, 0, 64);

  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}