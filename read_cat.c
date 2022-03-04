#include "type.h"

int readfile(int fd, int nbytes)
{
    char buf[BLKSIZE];
    OFT *temp;

    // Getting the file opened in the current process OFT list.
    temp = running->fd[fd];

    // If some file is opened at that file descriptor.
    if(temp)
    {
        if(temp->mode != 0 && temp->mode != 2)
        {
            printf("The file descriptor entered is not opened for R or RW\n");
            return -1;
        }
    }
    else
    {
        printf("The file descriptor entered is not opened.\n");
        return -1;
    }

    // Calling the myRead for reading the text from the file into the buf.
    int bytesRead = (myRead(fd, buf, nbytes));
    printf("\n*************** READ RESULTS **************\n");

    // Making sure that the buf is ending with a null char.
    buf[bytesRead] = 0;
    printf("%s", buf);
    printf("\n*************** END READ RESULTS **************\n");
    return bytesRead;
}

int myRead(int fd, char buf[], int nbytes)
{
    int count = 0, lbk, startByte, blk, ibuf[256], dbuf[256], tempBytes = nbytes;
    
    // OFT structure of the file.
    OFT * oftp = running->fd[fd];

    // Minode of the file.
    MINODE *mip = oftp->minodeptr;

    // At this moment, the file has available bytes to read.
    int avil = mip->INODE.i_size - oftp->offset;

    int remain;
    char *maxBuf = buf, readBuf[BLKSIZE]; // cq points at buf[ ]


    // Read from the file until there are bytes available to read.
    while(nbytes && avil)
    {
        // Calculating the local block.
        lbk = oftp->offset / BLKSIZE;

        // Calculating the start byte in the block.
        startByte = oftp->offset % BLKSIZE;

        // if the calculated logical block is within the direct blocks
        if (lbk < 12)
        {
            blk = mip->INODE.i_block[lbk];
            // map LOGICAL lbk to PHYSICAL blk
            get_block(mip->dev, blk, readBuf);
        }
        // Local block is indirect blocks.
        else if (lbk >= 12 && lbk < 256 + 12)
        {
            // map LOGICAL lbk to PHYSICAL blk
            get_block(mip->dev, mip->INODE.i_block[12], ibuf);
            blk = (lbk - 12);

            get_block(mip->dev, ibuf[blk], readBuf);
        }

        // Local block is double indirect block.
        else
        {
            get_block(mip->dev, mip->INODE.i_block[13], dbuf);

            // Mailman's algorithm.
            blk = ((lbk - 268) / 256);

            get_block(mip->dev, dbuf[blk], ibuf);

            blk = ((lbk - 268) % 256);

            get_block(mip->dev, ibuf[blk], readBuf);
        }

        // cp points at startByte in readbuf[]
        // Point cp from where we have to start reading.
       char *cp = readBuf + startByte;
       remain = BLKSIZE - startByte;   // number of bytes remain in readbuf[]

        // available bytes are >= Bytes to read.
        if(avil >= nbytes)
        {
            strncpy(maxBuf, cp, nbytes);
            maxBuf += nbytes;
            cp += nbytes;
            remain -= nbytes;

            // Updating the offset after reading the byte.
            oftp->offset += nbytes;

            // if one data block is not enough, for reading
            if(oftp->offset > mip->INODE.i_size)
            {
                mip->INODE.i_size += nbytes;
            }

            nbytes -= nbytes;
            return tempBytes;
        }

        // If the available bytes is less than the remaining bytes i.e.
        // we use remaining bytes left in the block to read.
        else
        {
            strncpy(maxBuf, cp, remain);
            maxBuf += remain;
            cp += remain;

            oftp->offset += avil;
            
            // if one data block is not enough, for reading
            if(oftp->offset > mip->INODE.i_size)
            {
                mip->INODE.i_size += avil;
            }

            if(avil <= remain)
            {
                tempBytes = avil;
                return tempBytes;
            }

            return remain;
        }


    }
   return 0;   // count is the actual number of bytes read

}

int myCat(char *path)
{
    char myBuf[BLKSIZE];
    int n = 0, fd = -1;

    path[strlen(path)] = '\0';

    printf("\nFile name: %s\n", path);

    // Opening the file.
    fd = my_open(path, "0");
    
    // If the file is not opened.
    if(fd == -1)
    {
        return -1;
    }
    else
    {
        printf("\n Open successful.");
    }

    printf("\n*************** FILE CONTENTS ***************\n");

    // If the file is not opened.
    while(n = myRead(fd, myBuf, 1024)) // Read n bytes.
    {
        myBuf[n] = 0;
        printf("%s", myBuf);
        memset(myBuf, 0, 1024);
    }
    printf("\n*************** END FILE CONTENTS ***************\n");

    closeFile(fd);
}