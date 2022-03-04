#ifndef TYPE_H
#define TYPE_H

/*************** type.h file for LEVEL-1 ****************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1
#define NFD         10      // Number of file descriptors

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2

#define SUPER_MAGIC 0xEF53



typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified
  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;



typedef struct mtable{

  int dev; // device number; 0 for FREE
  int ninodes; // from superblock
  int nblocks;
  int free_blocks; // from superblock and GD
  int free_inodes;
  int bmap; // from group descriptor
  int imap;
  int iblock; // inodes start block
  MINODE *mntDirPtr; // mount point DIR pointer
  char devName[64]; //device name
  char mntName[64]; // mount point DIR name

}MTABLE;

// Open File Table
typedef struct Oft{
  int   mode;
  int   refCount;
  MINODE *minodeptr;
  int  offset;
} OFT;

OFT oft[64];    // oft structs 

typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  int          status;
  MINODE      *cwd;      // CWD directory pointer

  OFT   *fd[NFD];

}PROC;



/*
      FUNCTION LIST
*/

int cd();
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls();
char *pwd(MINODE *wd);
int get_block(int dev, int blk, char *buf);
int put_block(int dev, int blk, char *buf);
int tokenize(char *pathname);
MINODE *iget(int dev, int ino);
void iput(MINODE *mip);
int search(MINODE *mip, char *name);
int getino(char *pathname);
int findmyname(MINODE *parent, u32 myino, char myname[ ]);
int findino(MINODE *mip, int *myino, int *parentino);

//Mkdir functions.
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
void decFreeInodes(int dev);
void decFreeBlocks(int dev);
int balloc(int dev);
int ialloc(int dev);
int aux_mkdir(MINODE* pmip, char *basename);
int my_mkdir();
int enter_name(MINODE *mp, int ino, char*name);

//Creat functions:
int enter_name_Creat(MINODE *pmip, int ino, char*name);
int kcreat(MINODE* pmip, char *basename);
int my_creat();

// Link Unlink functions.
int my_link(char *old, char* new);
int my_unlink(char *path);

// Symlink, Readlink functions
int my_symlink(char* old, char *new);


int truncate(MINODE *mip);
int my_open(char *filepath, char*mode);
void pfd();
int mylseek(int fd, int position);
int closeFile(int fd);

int readfile(int fd, int nbytes);
int myRead(int fd, char buf[], int nbytes);
int myCat(char *path);
int writefile();
#endif