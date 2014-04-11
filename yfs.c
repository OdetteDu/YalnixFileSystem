#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

void printInode(int level, char *where, struct inode *inode)
{
	//Print basic information about an inode
	TracePrintf(level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode ->type, inode ->nlink, inode->size, inode->indirect);
	
	//Print direct block
	TracePrintf(level, "Direct block: \n");
	int i;
	for(i=0; i<NUM_DIRECT; i++)
	{
		  TracePrintf(level, "%d\n", inode ->direct[i]);
	}
	TracePrintf(level, "\n");

	//Print indirect block
	if(inode -> indirect != 0)
	{
		char *buf;
		buf= malloc(sizeof(char) * SECTORSIZE);
		int readIndirectBlockStatus;
		readIndirectBlockStatus = ReadSector(inode ->indirect, buf);
		if( readIndirectBlockStatus != 0)
		{
			TracePrintf(0, "[Error @ %s @ printInode]: Read indirect block %d unsuccessfully\n", where, inode ->indirect);
		}
		else
		{
			TracePrintf(level, "Indirect block: \n");
			for(i=0; i<sizeof(char) * SECTORSIZE / sizeof(int); i++)
			{
				TracePrintf(level, "%d\n", (int *)buf+i);
			}
		}
	}

	TracePrintf(level, "\n");
}

void printDisk(int level, char *where)
{
	TracePrintf(level, "[Testing @ %s]: Print Disk:\n", where);

	//Read block 1
	char *buf;
	buf= malloc(sizeof(char) * SECTORSIZE);
	int readIndirectBlockStatus;
	readIndirectBlockStatus = ReadSector(1, buf);
	if( readIndirectBlockStatus != 0)
	{
		TracePrintf(0, "[Error @ %s @ printDisk]: Read indirect block %d unsuccessfully\n", where, 1);
		return;
	}

	//Print fs_header
	struct fs_header *fsHeader = (struct fs_header *)buf;
	TracePrintf(level, "fs_header: num_blocks(%d), num_inodes(%d)\n", fsHeader ->num_blocks, fsHeader ->num_inodes);

	//Print inodesi in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader ->num_inodes) + 1)/(BLOCKSIZE/INODESIZE);
	TracePrintf(level, "inodes in %d blocks:\n", numOfBlocksContainingInodes);
	int i;
	for(i=1; i<BLOCKSIZE/INODESIZE -1; i++)
	{
		  printInode(level, "", (struct inode *)buf+i);
	}

	//Print inodes not in block 1
	int blockIndex;
	for(blockIndex = 2; blockIndex < numOfBlocksContainingInodes +1; blockIndex ++)
	{
		readIndirectBlockStatus = ReadSector(blockIndex, buf);
		if( readIndirectBlockStatus != 0)
		{
			TracePrintf(0, "[Error @ %s @ printDisk]: Read indirect block %d unsuccessfully\n", where, blockIndex);
		}
		else
		{
			for(i=0; i<BLOCKSIZE/INODESIZE -1; i++)
			{
				  printInode(level, "", (struct inode *)buf+i);
			}
		}
	}

	TracePrintf(level, "\n");
}

int main(int argc, char **argv)
{
	int registerStatus = Register(FILE_SERVER);
	if(registerStatus != 0)
	{
		TracePrintf(0, "[Error @ yfs.main]: unsuccessfully register the YFS as the FILE_SERVER.\n");
	}

	printDisk(1000, "main.c");

	return 0;
}
