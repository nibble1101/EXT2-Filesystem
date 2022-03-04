#include "type.h"

int my_symlink(char *oldFileName, char *newFileName)
{
    int oldINO, newINO, pino;
    char temp[256], oldParentName[256], newParentName[256], newChildName[256], oldChildName[256];
    MINODE *oldmip, *pip, *newmip;

    if(oldFileName[0] == '/')
    {
        dev = root->dev;
    }
    else
    {
        dev = running->cwd->dev;
    }

    // Parse the all child names and parent names in paths
    strcpy(temp, oldFileName);
    strcpy(oldParentName, dirname(temp));
    strcpy(temp, oldFileName);
    strcpy(oldChildName, basename(temp));
    strcpy(temp, newFileName);
    strcpy(newParentName, dirname(temp));
    strcpy(temp, newFileName);
    strcpy(newChildName, basename(temp));

    // Get ino of oldFileName
    oldINO = getino(oldFileName);

    if(oldINO == 0)
    {
        printf("%s does not exist\n", oldFileName);
        return -1;
    }

    // Get oldFileName inode into memory
    oldmip = iget(dev, oldINO);

    // Get ino of parent (newFileName)
    pino = getino(newParentName);

    if(pino == 0)
    {
        printf("Dir %s does not exist\n", newParentName);
    }

    // Get parent directory inode (block of newFileName
    pip = iget(dev, pino);


     // If the node is not reg or a lnk
    if(!S_ISREG(oldmip->INODE.i_mode))
    {
        printf("Error: Cannot link to a directory\n");
        return -1;
    }

    // Make sure new file does not already exist
    if(search(pip, newChildName) != 0)
    {
        printf("Error: The file you are trying to add to path '%s' already exists\n", newParentName);
        return -1;
    }
    if(oldmip->dev != pip->dev)
    {
        printf("Error: paths '%s' and '%s' are not on the same device\n", oldFileName, newFileName);
        return -1;
    }

    // Create the file.
    my_creat(newFileName);


    // Get the new file's inode into memory.
    newINO = search(pip, newChildName);
    newmip = iget(dev, newINO);

    printf("mode before: %x\n", newmip->INODE.i_mode);

    newmip->INODE.i_mode = 0xA;
    newmip->INODE.i_size = strlen(oldChildName);

    // Copy the new child name to the block of the new file
    strcpy(temp, oldChildName);
    strcpy((char *)newmip->INODE.i_block, temp);

    newmip->dirty = 1;
    iput(newmip);
    pip->dirty = 1;
    iput(pip);

	return 1;
}