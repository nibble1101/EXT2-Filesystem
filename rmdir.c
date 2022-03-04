#include"type.h"

int my_rmdir(char *pathname)
{
	// Varibale declarations
	int ino = 0;
	char parent[64], temp[64];
	int parent_ino = 0;
	MINODE *mip = NULL;
	MINODE *parent_mip = NULL;
	char name[256];
	u32 tmp1, tmp2;

	// Get inode of pathname in memory.
	ino = getino(pathname);
	mip = iget(dev, ino);

	if (is_valid_directory(mip) == 0)
	{
		iput(mip);	// Important for dereferencing reference count.
		return -1;
	}
			

	strcpy(temp,pathname);
	strcpy(parent,dirname(temp));

	parent_ino = getino(parent);
	parent_mip = iget(mip->dev, parent_ino);

	// Get the name of the directory.
	//findmyname(parent_mip, ino, name);
	strcpy(temp,pathname);
	strcpy(name,basename(temp));
	rm_child(parent_mip, name);

	iput(parent_mip);

	// Free the block and the inode of deleted directory.
	bdalloc(mip->dev, mip->INODE.i_block[0]);
	idalloc(mip->dev, mip->ino);

	iput(mip);
	iput(parent_mip);

	return 1;
}

void rm_child(MINODE *parent_mip, char *file_name)
{
	INODE *parent = &parent_mip->INODE;
	int tempIno = 0;
	char buf[BLKSIZE];
	char *cp = NULL;
	int block = -1;
	int pos = 0;
	int total_entries = get_num_links(parent_mip);
	DIR *prevDir = NULL;

	// Extracting the block number as well as the position of the directory.
	for (int i = 0; i < 12; i++) 
	{
		if (parent->i_block[i] == 0) 
		{
			break;
		}
		// storing i_block[i] into tempIno
		tempIno = parent->i_block[i];

		// step to the last entry in the data block
		get_block(dev, parent->i_block[i], buf);

		// Set dp to first entry in dir (typically .).
		dp = (DIR *)buf;
		cp = buf;
		while (cp + dp->rec_len <= buf + BLKSIZE)
		{
			pos++;
			if (strcmp(dp->name, file_name) == 0) 
			{
				block = parent->i_block[i];
				break;
			}
			prevDir = dp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
	}

	if (pos == 0) 
	{
		bdalloc(dev, block);
		parent_mip->INODE.i_size = parent_mip->INODE.i_size - BLKSIZE;

		// Compacting the parent data blocks if the block deallocated is
		// between non zero enteries.
		adjust_iblock_array(parent_mip);
	} 
	
	else if (pos == total_entries) 
	{
		prevDir->rec_len += dp->rec_len;
	}
	
	else 
	{
		cp = buf;
		while (cp + dp->rec_len < buf + BLKSIZE) 
		{
			if (strcmp(dp->name, file_name) == 0) 
			{
				break;
			}
			prevDir = dp;
			cp += dp->rec_len;
			dp = (DIR *)cp;
		}
		cp += dp->rec_len;
		
		memcpy(dp, cp, dp->rec_len);
		prevDir->rec_len += dp->rec_len;
	}

	put_block(dev, tempIno, buf);
}

void adjust_iblock_array(MINODE *parent_mip)
{
	// Compacting the parent data blocks if the block deallocated is
	// between non zero enteries.

	int tmp = 0;
	for (int i = 0; i < 11; i++) {
			if (parent_mip->INODE.i_block[i] == 0 && parent_mip->INODE.i_block[i + 1] > 0) {
					tmp = parent_mip->INODE.i_block[i];
					parent_mip->INODE.i_block[i] = parent_mip->INODE.i_block[i + 1];
					parent_mip->INODE.i_block[i + 1] = 0;
			}
	}
}

int is_valid_directory(MINODE *mip)
{
	if (!S_ISDIR(mip->INODE.i_mode)) 
	{
		printf(" Error rmdir: Not a directory.\n");
		return 0;
	}

	if (mip->refCount > 1) 
	{
		printf("Error rmdir: Directory busy.\n");
		return 0;
	}

	if (get_num_links(mip) > 2) 
	{
		printf("Error rmdir: Trying to remove non-empty directory.\n");
		return 0;
	}

	return 1;
}

int get_num_links(MINODE *mip)
{
	int link_count = 1;
	INODE *parent = &mip->INODE;
	int tempIno = 0;
	char buf[BLKSIZE];
	char *cp = NULL;

	for (int i = 0; i < 12; i++) 
	{

		if (parent->i_block[i] == 0)
		{
			break;
		}
		
		// storing i_block[i] into tempIno
		tempIno = parent->i_block[i];

		// step to the last entry in the data block
		get_block(dev, parent->i_block[i], buf);

		dp = (DIR *)buf; // Set dp to first entry in dir (typically .).
		cp = buf;

		while (cp + dp->rec_len < buf + BLKSIZE)
		{
			cp += dp->rec_len;
			dp = (DIR *)cp;
			link_count++;
		}
	}

    return link_count;
}