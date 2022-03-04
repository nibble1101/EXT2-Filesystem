#include"type.h"

int my_mkdir(char *path)
{
    // temporary pathname varibale.
    char tempPathname[BLKSIZE];
    strcpy(tempPathname,path);

    // Make sure user entered pathname.
    if (strlen(path) == 0) 
    {
        printf("No specified pathname!\n");
        return 0;
    }

    // Separating the dirname and basename from the pathname.
    tokenize(path);
    char basename[128], dirname[128];
    int pino;       // Parent Inode number.

    // If the pathname has only one argument i.e pathname is actually the name of the directory.
    if(n == 1)
    {
        strcpy(basename,name[0]);
        printf("\nBasename: %s\n",basename);
        pino = running->cwd->ino;
    }
    else
    {
        // Extracting the basename and the parent directory path.
        strcpy(basename,name[n-1]);
        int size = strlen(pathname) - strlen(basename) -1;
        memmove(dirname, pathname,size);
        dirname[strlen(dirname)] = '\0';
        printf("\nDirectory: %s",dirname);
        printf("\nBasename: %s\n",basename);

        // Getting the parent Inode number.
        pino = getino(dirname);

        // Checking if the parent path is valid.
        if(pino == 0)
        {
            printf("Not a valid path.\n");
            return 0;
        }
    }
    
    // Getting the parent Inode in the Minode.
    MINODE* pmip = iget(dev,pino);
    printf("\nThe Pino = %d", pino);

    // Checking if the Minode is a directory?
    if(S_ISDIR(pmip->INODE.i_mode))
    {
        // Search for the basename in the directory as the basename
        // should not exist in the directory
        if(search(pmip,basename) == 0)
        {
            int value = aux_mkdir(pmip, basename);

            // if mkdir failed
            if(value == 0)
            {
                iput(pmip);     // Dereferencing
                return 0;
            }
            else if(value == 1)
            {
                iput(pmip);     // Dereferencing
                return 1;
            }

        }
        else
        {
            iput(pmip);         // Dereferencing
            return 0;
        }
    }
    else
    {
        iput(pmip);         // Dereferencing
        printf("\n****The file type is not directory.****\n");
        return 0;
    }
}

int aux_mkdir(MINODE* pmip, char *basename)
{
    // Step 1: Allocate the inode and block from the free inodes and blocks.
    int ino = ialloc(pmip->dev);
    int blk = balloc(pmip->dev);

    // Step 2: In this step we create an Inode and then we write it on the disk.
    MINODE *mip = iget(pmip->dev, ino);
    printf("\nino %d", ino);
    printf("\nblk: %d",blk);

    INODE *ip = &mip->INODE;
    ip->i_mode = 0x41ED; // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = BLKSIZE; // size in bytes
    ip->i_links_count = 2; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks hence 512*2 = 1024
    ip->i_block[0] = blk; // new DIR has one data block
    for(int i = 1; i < 15; i++)
    {
        ip->i_block[i] = 0;         // ip->i_block[1] to ip->i_block[14] = 0;
    }

    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk


    // Step 3: In this step we set name of the the parent directory of the Inode and the self directory Inode.
    char buf[BLKSIZE], *cp;
    bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
    // get_block(running->cwd->dev, blk, buf);
    DIR *dp = (DIR *)buf;
    // make . entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    strncpy(dp->name, ".", 1);
    // make .. entry: pino=parent DIR ino, blk=allocated block
    cp = buf + dp->rec_len;
	dp = (DIR *)cp;
    dp->inode = pmip->ino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    strncpy(dp->name, "..", 2);
    put_block(pmip->dev, blk, buf); // write to blk on diks

    // Step 4: Enter the new directory into the parent directory:
    return enter_name(pmip, ino, basename);
}

// Entering the name of the new directory into parent directory.
int enter_name(MINODE *pmip, int ino, char*name)
{
    int i = 0;
	// looping through each data block of parent directory.
    // We're finding the first empty block.
    for (i = 0; i < 12; i++) 
    {
        if (&pmip->INODE.i_block[i] == 0) 
        {
            break;
        }
        char buf[BLKSIZE];
        bzero(buf, BLKSIZE);

        // when entering a new entry with name_len = n
        int need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4

        // Getting the parent's datablock.
        get_block(pmip->dev, pmip->INODE.i_block[i], buf);
        dp = (DIR *)buf;
        char* cp = buf;

        // step to the last entry in a data block
        while(cp + dp->rec_len < buf + BLKSIZE)
        {
            //rec_length += dp->rec_len;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        cp = (char *)dp;
        // step to the last entry in a data block...its ideal length is...
        int ideal_length = 4 *((8 + dp->name_len + 3) / 4);
        int remain = dp->rec_len - ideal_length;// store rec_len for a bit easier code writing

        // check if it can enter the new entry as the last entry
        if(remain >= need_length)
        {
            // trim the previous entry to its ideal_length
            dp->rec_len = ideal_length;
            cp+=dp->rec_len;
            dp = (DIR *)cp;
            dp->rec_len = remain;
            dp->name_len = strlen(name);
            strncpy(dp->name, name, strlen(name));
            dp->inode = ino;
            // dp->file_type = EXT2_FT_DIR;

            // write the new block back to the disk
            put_block(pmip->dev, pmip->INODE.i_block[i], buf);


            // inc parent inode's link count by 1, touch its atime, and mark it dirty
            pmip->INODE.i_links_count++;
            pmip->INODE.i_atime = time(0L);
            pmip->dirty = 1;
            iput(pmip); // cleanup

            return 1;
        }
    }

    char buf2[BLKSIZE];
    bzero(buf2,BLKSIZE);
    // otherwise allocate a new data block 
    i++;
    int blk = balloc(dev);
    pmip->INODE.i_block[i] = blk;
    get_block(dev, blk, buf2);

    // enter the new entry as the first entry in the new block
    DIR* dirp = (DIR *)buf2;
    dirp->rec_len = BLKSIZE;
    dirp->name_len = strlen(name);
    strncpy(dirp->name, name, dirp->name_len);
    dirp->inode = ino;
    dirp->file_type = EXT2_FT_DIR;

    pmip->INODE.i_size += BLKSIZE;
    
    // write the new block back to the disk
    put_block(dev, pmip->INODE.i_block[i], buf2);

    // inc parent inode's link count by 1, touch its atime, and mark it dirty
	pmip->INODE.i_links_count++;
    
	pmip->INODE.i_atime = time(0L);
	pmip->dirty = 1;
	iput(pmip); // cleanup
    if(pmip->INODE.i_links_count > 0)
    {
        iput(pmip); // cleanup
        return 1;
    }
	    
    else
        return 0;
}



int my_creat(char *path)
{
    // Separating the dirname and basename from the pathname.
    tokenize(path);
    char basename[128], dirname[128], gpath[128];
    int pino;

    // If the pathname has only one argument i.e pathname is actually the name of the directory.
    if(n == 1)
    {
        strcpy(basename,name[0]);
        pino = running->cwd->ino;
    }
    else
    {
        strcpy(basename,name[n-1]);
        int size = strlen(path) - strlen(basename)-1;
        memmove(dirname, path,size);
        dirname[size] = '\0';
        // Getting the Inode number.
        pino = getino(dirname);
        if(pino == 0)
        {
            return 0;
        }
    }
    
    // Getting the Inode in the Minode.
    MINODE* pmip = iget(dev,pino);

    // Checking if the Minode is a directory?
    if(S_ISDIR(pmip->INODE.i_mode))
    {
        // Search for the basename in the directory.
        if(search(pmip,basename) == 0)
        {
            return kcreat(pmip, basename);
        }
        else
        {
            printf("\n****The file %s already exists.****", basename);
            return 0;
        }
    }
    else
    {
        printf("\n****The file type is not directory.****\n");
        return 0;
    }
}

int kcreat(MINODE* pmip, char *basename)
{
    // Step 1: Allocate the inode from the free inodes.
    int ino = ialloc(dev);

    // Step 2: In this step we create an Inode and then we write it on the disk.
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0x81A4; // 040755: DIR type and permissions
    ip->i_uid = running->uid; // owner uid
    ip->i_gid = running->gid; // group Id
    ip->i_size = 0; // size in bytes
    ip->i_links_count = 1; // links count=2 because of . and ..
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
    ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks hence 512*2 = 1024

    for(int i = 0; i < 12; i++)
    {
        ip->i_block[i] = 0;         // ip->i_block[1] to ip->i_block[14] = 0;
    }
    mip->dirty = 1; // mark minode dirty
    iput(mip); // write INODE to disk


    // Step 3: There is no step 3 as the file doesn't have the parent directory.

    // Step 4: Enter the new directory into the parent directory:
    return enter_name_Creat(pmip, ino, basename);
}

int enter_name_Creat(MINODE *pmip, int ino, char*name)
{
    int i = 0;

    // Get to the second last block.
	while(pmip->INODE.i_block[i])
		i++;
	i--;

    char buf[BLKSIZE];
    bzero(buf, BLKSIZE);

    // Getting the parent's datablock.
    get_block(pmip->dev, pmip->INODE.i_block[i], buf);
	dp = (DIR *)buf;
	char* cp = buf;
	int rec_length = 0;

    // step to the last entry in a data block
	while(dp->rec_len + rec_length < BLKSIZE)
	{
		rec_length += dp->rec_len;
		cp += dp->rec_len;
		dp = (DIR *)cp;
	}

    // when entering a new entry with name_len = n
    // name is the name of the child.
	int need_length = 4 * ((8 + strlen(name) + 3) / 4); // a multiple of 4

	// step to the last entry in a data block...its ideal length is...
	int ideal_length = 4 *((8 + dp->name_len + 3) / 4);

	rec_length = dp->rec_len; // store rec_len for a bit easier code writing

    // check if it can enter the new entry as the last entry
	if((rec_length - ideal_length) >= need_length)
	{
		// trim the previous entry to its ideal_length
		dp->rec_len = ideal_length;
		cp+=dp->rec_len;
		dp = (DIR *)cp;
		dp->rec_len = rec_length - ideal_length;
		dp->name_len = strlen(name);
		strncpy(dp->name, name, dp->name_len);
		dp->inode = ino;

		// write the new block back to the disk
		put_block(dev, pmip->INODE.i_block[i], buf);
	}

    else
	{
        char buf2[BLKSIZE];
        bzero(buf2,BLKSIZE);
		// otherwise allocate a new data block 
		i++;
		int blk = balloc(dev);
		pmip->INODE.i_block[i] = blk;
		get_block(dev, blk, buf2);

        pmip->INODE.i_size += BLKSIZE;

		// enter the new entry as the first entry in the new block
		DIR* dirp = (DIR *)buf2;
		dirp->rec_len = BLKSIZE;
		dirp->name_len = strlen(name);
		strncpy(dirp->name, name, dirp->name_len);
		dirp->inode = ino;
		
		// write the new block back to the disk
		put_block(dev, pmip->INODE.i_block[i], buf2);
	}

    // touch parent's atime and mark it dirty.
	pmip->INODE.i_atime = time(0L);
	pmip->dirty = 1;
	iput(pmip); // cleanup
	return 1;
}