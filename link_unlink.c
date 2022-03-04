#include "type.h"

int my_link(char *old, char *new)
{
	char temp[64];
	char link_parent[64], link_child[64];
	int oino;
	int p_ino;
	MINODE *omip;
	MINODE *p_mip;
	INODE *ip;
	INODE *p_ip;
	printf("\nOld File: %s\nNew File %s\n",old,new);
	//Checks
	if(!strcmp(old, ""))
	{
		printf("\nERROR: no old/new file Entered.");
		return 0;
	}

	//get oldfilename's inode
	oino = getino(old);

	//verify old file exists
	if(oino == 0)
	{
		printf("\nERROR: %s does not exist", old);
		return 0;
	}

	omip = iget(dev, oino);

	//Verify it is a file
	if(S_ISDIR(omip->INODE.i_mode))
	{
		printf("\nERROR: can't link directory");
		return 0;
	}
	printf("\nOld file found!");

	// Verifying new file doesn't exists.
	if(getino(new) > 0)
	{
		printf("\nError: New file already exists.");
		return 0;
	}

	strcpy(temp,new);

	// Getting parent directory name and the new file name.
	strcpy(link_child,basename(temp));
	strcpy(link_parent, dirname(temp));

	printf("\nafter stuff new is %s", new);

	//get new's parent minode
	p_ino = getino(link_parent);
	p_mip = iget(dev, p_ino);

	//verify that link parent exists
	if(!p_mip)
	{
		printf("\nERROR: no parent");
		return 0;
	}

	//verify link parent is a directory
	if(!S_ISDIR(p_mip->INODE.i_mode))
	{
		printf("ERROR: not a directory\n");
		return 0;
	}

	//verify that link child does not exist yet
	if(getino(new))
	{
		printf("ERROR: %s already exists\n", new);
		return 0;
	}

	//Enter the name for the newfile into the parent dir
	printf("entering name for %s\n", link_child);
	//This ino is the ino of the old file
	//This is how it is linked

	enter_name_Creat(p_mip, oino, link_child);

	ip = &omip->INODE;

	//increment the link count
	ip->i_links_count++;
	omip->dirty = 1;

	p_ip = &p_mip->INODE;

	p_ip->i_atime = time(0L);
	//Set dirty and iput
	p_mip->dirty = 1;

	iput(p_mip);

	iput(omip);

	printf("end of link\n");
	return 1;
}


int my_unlink(char *path)
{
	int ino, i;
	int parent_ino;
	MINODE *mip;
	MINODE *parent_mip;
	INODE *ip;
	INODE *parent_ip;

	char temp[64];
	char parent[64];
	char child[64];

	//Checks
	if(!path)
	{
		printf("\nERROR: no path given");
		return 0;
	}

	//Gets the ino and checks to ensure it exists
	ino = getino(path);

	if(ino == 0)
	{
		printf("\nERROR: bad path");
		return 0;
	}

	//Get the minode and check to make sure that it is a file
	mip = iget(dev, ino);

	if(!mip)
	{
		printf("\nERROR: missing minode");
		return 0;
	}

	//Make sure its a file
	if(S_ISDIR(mip->INODE.i_mode))
	{
		printf("ERROR: can't unlink a directory\n");
		return 0;
	}

	printf("\ndoing the unlinking");

	// Copying the path into temp.
	strcpy(temp, path);

	// Getting the parent directory name.
	strcpy(parent, dirname(temp));

	// Copying the path into temp.
	strcpy(temp, path);

	// Getting the child directory name.
	strcpy(child, basename(temp));

	//Gets the parent and removes the file from its parent
	parent_ino = getino(parent);
	parent_mip = iget(dev, parent_ino);

	printf("\ndirname is %s basename is %s", parent, child);

	//Removes the child from the parent
	printf("removing %s from %s\n", parent, child);

	// Removing the child from parent's data block.
	rm_child(parent_mip, child);

	// Marking the parent minode as dirty after removing the child.
	parent_mip->dirty = 1;

	// Getting the Inode of the parent minode.
	parent_ip = &parent_mip->INODE;

	//Update the time, set dirty, and iput
	parent_ip->i_atime = time(0L);
	parent_ip->i_mtime = time(0L);

	// iput the parent minode.
	iput(parent_mip);

	// Getting the inode of the mip.
	ip = &mip->INODE;

	//Decrement link count
	ip->i_links_count--;
	printf("links: %d\n", ip->i_links_count);

	if(ip->i_links_count > 0)
	{
		mip->dirty = 1;
	}

	else
	{
		//Deallocate its blocks
		for(i = 0; i < 12 && ip->i_block[i] != 0; i++)
		{
			bdalloc(dev, ip->i_block[i]);
		}

		//deallocate its inode
		idalloc(dev, ino);
	}

	iput(mip);

	return 1;
}