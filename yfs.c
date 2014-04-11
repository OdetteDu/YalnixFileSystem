#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

int main(int argc, char **argv)
{
	int registerStatus = Register(FILE_SERVER);
	if(registerStatus != 0)
	{
		TracePrintf(0, "[Error @ yfs.main]: unsuccessfully register the YFS as the FILE_SERVER.\n");
	}

	return 0;
}

void printInode(int level, char *where, struct inode *inode)
{
	//Print basic information about an inode
	TracePrintf(level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode ->type, inode ->nlink, inode->size, inode->indirect);
	
	//Print direct block
	int i;
	for(i=0; i<NUM_DIRECT; i++)
	{
		  TracePrintf(level, "%d ", inode ->direct[i]);
	}

	//Print indirect block
	char *buf = malloc(sizeof(char) * SECTORSIZE);
	int readIndirectBlockStatus;
	readIndirectBlockStatus = ReadSector(inode ->indirect, buf);
	if( readIndirectBlockStatus != 0)
	{
		TracePrintf(0, "[Error @ %s @ printInode]: Read indirect block unsuccessfully\n", where);
	}



	TracePrintf(level, "\n");
}
