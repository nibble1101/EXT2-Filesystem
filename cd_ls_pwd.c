/************* cd_ls_pwd.c file **************/
#include"type.h"

int cd()
{
  // printf("cd: under construction READ textbook!!!!\n");

  // Get the inode number from the pathname.
  int ino = getino(pathname);

  // Getting the memory inode.
  MINODE *mip = iget(dev, ino);

  // Checking if it's a directory.
  if(S_ISDIR(mip->INODE.i_mode))
  {
    // Changing the current directory to desired mip.
    running->cwd = mip;

    // Iput the mip for derefernecing.
    iput(running->cwd);
    printf("\nCD successful.\n");

  }
  else
  {
    // Iput the mip for derefernecing.
    iput(mip);
    printf("\n****The file type is not directory.****\n");
  }
}

int ls_file(MINODE *mip, char *name)
{
  INODE *ip;
	char c, *time, dir[256];

	ip = &mip->INODE;

	// Print file mode
	if(S_ISDIR(ip->i_mode))
		printf("d");
	else if (S_ISREG(ip->i_mode))
		printf("-");
	else if(S_ISLNK(ip->i_mode))
		printf("l");
	else
		printf("-");

  // Permissions checking
	for (int i = 8; i >= 0; i--)
	{
		if (i % 3 == 2)
		{
			if (ip->i_mode & (1 << i))
				c = 'r';
			else
				c = '-';
		}
		if (i % 3 == 1)
		{
			if (ip->i_mode & (1 << i))
				c = 'w';
			else
				c = '-';
		}
		if (i % 3 == 0)
		{
			if (ip->i_mode & (1 << i))
				c = 'x';
			else
				c = '-';
		}

		putchar(c);
	}
	// Other info/name printing
	printf(" %d %d %d %.4d", ip->i_links_count, ip->i_uid, ip->i_gid, ip->i_size);
	time = ctime(&(ip->i_mtime));
	time[strlen(time)-1] = '\0';
	printf(" %s ", time);
  printf(" %s\n", name);
}

int ls_dir(MINODE *mip)
{
  // printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

  // Declaring the varibales.
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  int ino;

  // Getting the first block of memory inode.
  get_block(dev, mip->INODE.i_block[0], buf);
  dp = (DIR *)buf;    // dp = directory pointer.
  cp = buf;           // cp = current pointer.
  
  while (cp < buf + BLKSIZE){
    
    // Copying the file name in temp.
    strncpy(temp, dp->name, dp->name_len);

    // Making sure that the file name is terminated with the null character.
    temp[dp->name_len] = 0;
    ino = dp->inode;

    // Getting the inode of the file/directory.
    mip = iget(dev,ino);

    // For printing the file information.
    ls_file(mip, temp);

    // Stepping through the next block.
    cp += dp->rec_len;
    dp = (DIR *)cp;
    iput(mip); // for dereferencing.
  }
  printf("\n");
}

int ls()
{
  // printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");

  // mip varibale for getting the memory inode.
  MINODE *mip;

  // If the pathname is null then the Memory inode will be the cwd inode.
  if(strlen(pathname) == 0)
  {
		mip = running->cwd;
    ls_dir(mip);
    return 0;
  }

  // If the absolute pathname is provided.
  else
  {
    if(pathname[0] == '/')
    {
      mip = root;
    }

    else
    {
      mip = running->cwd;
    }
  }

  printf("\npathname = %s", pathname);

  // Getting the inode number using the getino function from the pathname.
  int ino = getino(pathname);

  // Getting the memory inode using the inode number and iget function.
  mip = iget(dev, ino);

  // If the returned memory Inode is directory
  if(S_ISDIR(mip->INODE.i_mode))
    ls_dir(mip);
  else
  {
    printf("\n****The file type is not directory.****\n");
  }

  iput(mip);  // Dereferencing.
}

void rpwd(MINODE *wd, int tempIno)
{

  // Declaring variables.
  char buf[BLKSIZE], name[256];
	char *cp;
	DIR *dp;
	MINODE *parentMip;

  // Getting the first direct block.
	get_block(dev, wd->INODE.i_block[0], (char *)&buf);
	dp = (DIR *)buf;     // get first dir "."
	cp = buf + dp->rec_len;
	dp = (DIR *)cp;   // get second dir ".."

  // Making sure that the pwd is not called in the root directory.
	if (wd->ino != root->ino)
	{
    // Ino of parent ..
		int parentIno = dp->inode;

    // Getting the parent memory inode.
		parentMip = iget(dev, parentIno);

    // Calling the reccursive pwd with parent's inode.
		rpwd(parentMip, wd->ino);

    iput(parentMip);  // Dereferencing for the reference count.
	}

  // Once we're in the root directory
	if (tempIno != 0)
	{
		while(dp->inode != tempIno)
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;     // get second dir ".."
		}
		strncpy(name, dp->name, dp->name_len);
		name[dp->name_len] = '\0';
		printf("%s/", name);
	}
  
	return;
}
char *pwd(MINODE *wd)
{
  // printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd->ino == root->ino){
    printf("/\n");
    return;
  }
  else
  {
    printf("/");
    rpwd(wd,0);
  }
  printf("\n");
}



