#include "type.h"

int truncate(MINODE *mip)
{
    int i = 0, j = 0, ibuf[256], dbuf[256];

   // 1. release mip->INODE's data blocks;
    // a file may have 12 direct blocks, 256 indirect blocks and 256*256
    // double indirect data blocks. release them all.

    // 12 direct data blocks.
    // 0 to 11 blocks, release them directly.
    for(i = 0; i < 12; i++)
    {
        if(mip->INODE.i_block[i])
        {
            bdalloc(mip->dev, mip->INODE.i_block[i]);
            mip->INODE.i_block[i] = 0;
        }
    }

    // Getting the 12th block in ibuf.
    // Indirect block has 256 blocks in total.
    get_block(mip->dev, mip->INODE.i_block[12], ibuf);

    // Dealloc the indirect blocks. All 256
    if (mip->INODE.i_block[12])
    {
        for(i = 0; i < 256; i++)
        {
            if(ibuf[i])
            {
                printf("ibuf[%d] in truncate\n", ibuf[i]);
                bdalloc(mip->dev, ibuf[i]);
                ibuf[i] = 0;
            }
        }

        mip->INODE.i_block[12] = 0;
    }

    // Getting the indirect 13th block in dbuf.
    // Double Indirect block has 256*256 blocks in total.
    get_block(mip->dev, mip->INODE.i_block[13], dbuf);

    // Dealloc all 256 * 256 double indirect blocks
    if(mip->INODE.i_block[13])
    {
        // Free the double indirect blocks
        for(i = 0; i < 256; i++)
        {
            // Get 256 blocks of i, (i = 0 to 255) in ibuf
            get_block(mip->dev, dbuf[i], ibuf);

            if(dbuf[i])
            {

                for(j = 0; j < 256; j++)
                {
                    bdalloc(mip->dev, ibuf[j]);
                    ibuf[j] = 0;
                }
                
                bdalloc(mip->dev, dbuf[i]);
                dbuf[i] = 0;
            }

        }

        mip->INODE.i_block[13] = 0;
    }

    //   2. update INODE's time field
    mip->INODE.i_atime = mip->INODE.i_ctime = mip->INODE.i_mtime = time(0L);
    
    //   3. set INODE's size to 0 and mark Minode[ ] dirty
    mip->INODE.i_size = 0;
    mip->dirty = 1;
    iput(mip);

}

int my_open(char *filepath, char* filemode)
{
    // Declaring the varibales.

    int i, ino, fd = 0, perm = 0, mask = 0, mode = -1, offset = 0;
	MINODE *mip;
	INODE* ip;
	OFT* of = NULL;

    char buf[1024];

	printf("Opening file, %s\n",pathname);

    // Checking if the file mode has been entered.
    if(strlen(filemode) == 0)
    {
        printf("\nFile mode not entered.");
        return -1;
    }

    // Checking if the correct file mode has been entered and then storing it.
    if (!strcmp(filemode, "0"))                 // Read
		mode = 0;
	else if (!strcmp(filemode, "1"))            // Write    
		mode = 1;
	else if (!strcmp(filemode, "2"))            // Read Write
		mode = 2;
	else if (!strcmp(filemode, "3"))            // Append
		mode = 3;
	else
	{
		printf("Invalid mode!\n");
		return -1;
	}

    // Getting the inode number of the file.
    ino = getino(filepath);

    if(ino == 0)
    {
        printf("\n The file doesn't exists.");
        return -1;
    }

    // Getting the memory inode.
    mip = iget(dev, ino);

    // check mip->INODE.i_mode to verify it's a REGULAR file and permission OK
    if (!S_ISREG(mip->INODE.i_mode))
	{
		printf("\nNot a file!");
		iput(mip);
		return -1;
	}

    //check to see whether it is already open with an incompatable type
	for(i =0;i<NFD;i++)
	{
        // If the running process has an open file.
		if (running->fd[i] != 0)
		{
            // If the opened file minodeptr points to memory inode of the file tried to being open.
			if (running->fd[i]->minodeptr->ino == mip->ino)
			{
				if (running->fd[i]->mode>0)
				{
					printf("Error: File already opened for writing\n");
                    iput(mip);
					return -1;
				}
			}
		}
	}
    // allocate a FREE OpenFileTable (OFT) and fill in values
    OFT *oftp = (OFT *)malloc(sizeof(OFT));

	oftp->mode = mode;
	oftp->refCount = 1;
	oftp->minodeptr = mip;

    // Depending on the open mode 0|1|2|3, set the OFT's offset accordingly:
    switch(oftp->mode)
	{
		case 0:
			oftp->offset = 0;       // R: offset = 0
			break;
		case 1:
            truncate(mip);          // W: truncate file to 0 size
			oftp->offset = 0;
			break;
		case 2:
			oftp->offset = 0;       // RW: do NOT truncate file
			break;
		case 3:
			oftp->offset = mip->INODE.i_size;   // APPEND mode
			break;
		default:
			printf("invalid mode\n");
			return -1;
	}

    int smallesti = 0;

    for(i = 0; i < NFD; i++)
    {
        if(running->fd[i] == 0)
        {
            smallesti = i;
            break;
        }
    }

    running->fd[smallesti] = oftp;

    mip->INODE.i_atime = mip->INODE.i_mtime = time(0L);
    mip->dirty = 1;
    iput(mip);
    return smallesti;
}


// Prints data in running->oft
void pfd()
{
    int i = 0;
    // OFT *fd;
    printf("fd    mode    offset    INODE    SIZE\n");
    printf("--    ----    ------    -----    ----\n");

    for (i = 0; i < NFD; i++)
    {
        if(running->fd[i])
        {
            fd = running->fd[i];
            printf("%d       %d       %d      [%d, %d]   %d\n", i, running->fd[i]->mode, running->fd[i]->offset, running->fd[i]->minodeptr->dev, running->fd[i]->minodeptr->ino, running->fd[i]->minodeptr->INODE.i_size);
        }
    }
}

int mylseek(int fd, int position)
{
    // Lseek get the file descriptor and the position of the lseek.
    printf("FD: %d      Position: %d", fd, position);

    // FD should be within the range.
    if (fd < 0 || fd >= NFD)
    {
        printf("The specified file descriptor is out of range\n");
        return -1;
    }

    // Get the OFT data structure at the file descriptor.
    OFT *temp = running->fd[fd];

    // Get the original position.
    int originalPosition = running->fd[fd]->offset;

    // Position shouldn't be less than 0 or greater than the file size.
    if (position < 0 || position > temp->minodeptr->INODE.i_size)
    {
        printf("The specified position is not within range\n");
        return -1;
    }

    // Set the size.
    running->fd[fd]->offset = position;

    return originalPosition;
}


// Closes the file specified by its descriptor
int closeFile(int fd)
{
    OFT *oftp;
    MINODE *mip;
    if (fd < 0 || fd >= NFD)
    {
        printf("Error: The specified file descriptor is out of range\n");
        return -1;
    }

    if(running->fd[fd] == 0)
    {
        printf("Error: The file descriptor is null\n");
        return -1;
    }

    oftp = running->fd[fd];
    running->fd[fd] = 0;
    oftp->refCount--;
    if(oftp->refCount > 0){ return 0; }
    mip = oftp->minodeptr;
    iput(mip);

    printf("\n File has been closed.");
}
