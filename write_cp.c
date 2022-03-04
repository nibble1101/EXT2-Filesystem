# include "type.h"


int writefile()
{
    int nbytes, fd = -1;
    char buf[BLKSIZE];
    OFT *temp;

    printf("\nEnter the File Descriptor of the file: ");
    scanf("%d", &fd);

    if(fd < 0 && fd > NFD)
    {
        printf("\nThe file descriptor entered is invalid.");
    }

    // Getting the file opened in the current process OFT list.
    temp = running->fd[fd];

    // If some file is opened at that file descriptor.
    if(temp)
    {
        // verify fd is indeed opened for WR or RW or APPEND mode
        if(temp->mode != 1 && temp->mode != 2 && temp->mode != 3)
        {
            printf("\nThe file descriptor entered is not opened for W or RW or append.");
            return -1;
        }
    }

    else
    {
        printf("\nThe file descriptor entered is not opened.");
        return -1;
    }

    // copy the text string into a buf[] and get its length as nbytes.
    printf("\nEnter text to be written in the file: \n");
    scanf("%s", buf);
    buf[strlen(buf)] = '\0';
    //fgets(buf, 128, stdin);

    nbytes = strlen(buf);

    // Calling the myRead for reading the text from the file into the buf.
    return(myWrite(fd, buf, nbytes));
}

// Writes nbytes to the file given a descriptor
int myWrite(int fd, char buf[], int nbytes)
{
    // Declaring necessary varibales. 
    int lbk = 0, startByte = 0, blk = 0, remain = 0, tempBytes = nbytes, dblock, doffset, dindex;
    int ibuf[256];

    // Getting the OFT Data structure.
    OFT *oftp = running->fd[fd];

    // Getting the minode of the file.
    MINODE *mip = oftp->minodeptr;
    INODE *ip;
    char wbuf[BLKSIZE], *maxBuf = buf;
    memset(wbuf, 0, 1024);
    memset(ibuf, 0, 256);
    ip = &(mip->INODE);

    // Run the loop until no bytes are left to write on the file.
    while (nbytes > 0)
    {
        // Logical block and the start byte.
        lbk       = oftp->offset / BLKSIZE;
        startByte = oftp->offset % BLKSIZE;

        // Getting the correct block to start writing.
        if (lbk < 12)
        {                         // direct block
            if (mip->INODE.i_block[lbk] == 0)
            {
                // MUST ALLOCATE a block 
                mip->INODE.i_block[lbk] = balloc(mip->dev);                                    // but MUST for I or D blocks
            }

            // blk should be a disk block now
            blk = mip->INODE.i_block[lbk];  
        }
        else if (lbk >= 12 && lbk < 256 + 12)
        {   
            // If indirect block is not allocated.
            if(ip->i_block[12] == 0 && lbk == 12)
            {
                ip->i_block[12] = balloc(mip->dev);

                get_block(mip->dev, ip->i_block[12], ibuf);
                ibuf[0] = balloc(mip->dev);
                blk = ibuf[0];
                put_block(mip->dev, ip->i_block[12], ibuf);
            }
            else
            {
                get_block(mip->dev, ip->i_block[12], ibuf);

                // Allocating an empty block.
                if(ibuf[lbk - 12] == 0)
                {
                    ibuf[lbk - 12] = balloc(mip->dev);
                }

                blk = ibuf[lbk - 12];
                put_block(mip->dev, ip->i_block[12], ibuf);
            }
        }
        else
        {
            // calculate the double indirect block # and the indirect block index (Mailman's algorithm.)
            dindex = (lbk - 268) / 256;
            doffset = (lbk -268) % 256;

            // If the double indirect block is not allocated.
            if (ip->i_block[13] == 0)
            {
                // alloc a block
                ip->i_block[13] = balloc(dev);
            }

            // read the block into the int buf
            get_block(mip->dev, ip->i_block[13], (char *)ibuf);

            // if double indirect block is 0
            if (ibuf[dindex] == 0) 
            {
                ibuf[dindex] = balloc(dev); // alloc a block
            }

            // set block to indirect block # that the double indirect block points to
            dblock = ibuf[dindex]; 

            put_block(mip->dev, ip->i_block[13], (char *)ibuf); // write int buf back to disk
            get_block(mip->dev, dblock, (char *)ibuf); // get double indirect block int int buf

            if(ibuf[doffset] == 0) // if the indirect block is 0
            {
                ibuf[doffset] = balloc(dev); // alloc block
            }
            blk = ibuf[doffset]; // set block to the block that the indirect block points to

            put_block(mip->dev, dblock, (char *)ibuf); // write indirect block back to disk
        }

        /* all cases come to here : write to the data block */
        get_block(mip->dev, blk, wbuf);   // read disk block into wbuf[ ]
        char *cp = wbuf + startByte;      // cp points at startByte in wbuf[]
        remain = BLKSIZE - startByte;     // number of BYTEs remain in this block

        if(remain >= nbytes)
        {
            strncpy(cp, maxBuf, nbytes);
            maxBuf += nbytes;
            cp += nbytes;
            remain -= nbytes;
            oftp->offset += nbytes;
            if(oftp->offset > ip->i_size)
            {
                ip->i_size += nbytes;
            }
            nbytes -= nbytes;
        }
        else
        {
            strncpy(cp, maxBuf, remain);
            maxBuf += remain;
            cp += remain;
            nbytes -= remain;

            if(oftp->offset > ip->i_size)
            {
                ip->i_size += remain;
            }

                remain -= remain;
        }
        put_block(mip->dev, blk, wbuf);   // write wbuf[ ] to disk
    }

     // loop back to while to write more .... until nbytes are written
    mip->dirty = 1;       // mark mip dirty for iput()
    iput(mip);
    printf("wrote %d chars into file descriptor fd=%d\n", tempBytes, fd);
    return tempBytes;
}
