#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

#include "global.h"

struct Message *msg;
char *pathname;
char fileName[DIRNAMELEN];
int fileNameCount;

void printInode( int level, char *where, struct inode *inode )
{
	//Print basic information about an inode
	TracePrintf( level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode->type, inode->nlink, inode->size,
			inode->indirect );

	//Print direct block
	TracePrintf( level, "Direct block: \n" );
	int i;
	for( i = 0; i < NUM_DIRECT; i++ )
	{
		TracePrintf( level, "%d\n", inode->direct[i] );
	}
	TracePrintf( level, "\n" );

	//Print indirect block
	if( inode->indirect != 0 )
	{
		char *buf;
		buf = malloc( sizeof(char) * SECTORSIZE );
		int readIndirectBlockStatus;
		readIndirectBlockStatus = ReadSector( inode->indirect, buf );
		if( readIndirectBlockStatus != 0 )
		{
			TracePrintf( 0, "[Error @ %s @ printInode]: Read indirect block %d unsuccessfully\n", where, inode->indirect );
		}
		else
		{
			TracePrintf( level, "Indirect block: \n" );
			for( i = 0; i < sizeof(char) * SECTORSIZE / sizeof(int); i++ )
			{
				TracePrintf( level, "%d\n", (int *) buf + i );
			}
		}
	}

	TracePrintf( level, "\n" );
}

void printDisk( int level, char *where )
{
	TracePrintf( level, "[Testing @ %s]: Print Disk:\n", where );

	//Read block 1
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	int readIndirectBlockStatus;
	readIndirectBlockStatus = ReadSector( 1, buf );
	if( readIndirectBlockStatus != 0 )
	{
		TracePrintf( 0, "[Error @ %s @ printDisk]: Read indirect block %d unsuccessfully\n", where, 1 );
		return;
	}

	//Print fs_header
	struct fs_header *fsHeader = (struct fs_header *) buf;
	TracePrintf( level, "fs_header: num_blocks(%d), num_inodes(%d)\n", fsHeader->num_blocks, fsHeader->num_inodes );

	//Print inodesi in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "inodes in %d blocks:\n", numOfBlocksContainingInodes );
	int i;
	for( i = 1; i < BLOCKSIZE / INODESIZE - 1; i++ )
	{
		printInode( level, "", (struct inode *) buf + i );
	}

	//Print inodes not in block 1
	int blockIndex;
	for( blockIndex = 2; blockIndex < numOfBlocksContainingInodes + 1; blockIndex++ )
	{
		readIndirectBlockStatus = ReadSector( blockIndex, buf );
		if( readIndirectBlockStatus != 0 )
		{
			TracePrintf( 0, "[Error @ %s @ printDisk]: Read indirect block %d unsuccessfully\n", where, blockIndex );
		}
		else
		{
			for( i = 0; i < BLOCKSIZE / INODESIZE - 1; i++ )
			{
				printInode( level, "", (struct inode *) buf + i );
			}
		}
	}

	TracePrintf( level, "\n" );
}

//mark used blocks used by inode
int markUsedBlocks( struct inode *inode, int *isBlockFree )
{
	int level = 1100;
	char *where = "yfs.c @ markUserBlocks";

	//Print basic information about an inode
	TracePrintf( level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode->type, inode->nlink, inode->size,
			inode->indirect );

	if( inode->type == INODE_FREE )
	{
		return FREE; //return 0
	}
	else
	{
		//Print direct block
		TracePrintf( level, "Direct block: \n" );
		int i;
		for( i = 0; i < NUM_DIRECT; i++ )
		{
			TracePrintf( level, "%d\n", inode->direct[i] );
			isBlockFree[inode->direct[i]] = USED;
		}
		TracePrintf( level, "\n" );

		//Print indirect block
		if( inode->indirect != 0 )
		{
			char *buf;
			buf = malloc( sizeof(char) * SECTORSIZE );
			int readIndirectBlockStatus;
			readIndirectBlockStatus = ReadSector( inode->indirect, buf );
			if( readIndirectBlockStatus != 0 )
			{
				TracePrintf( 0, "[Error @ %s @ printInode]: Read indirect block %d unsuccessfully\n", where, inode->indirect );
				return ERROR;
			}

			TracePrintf( level, "Indirect block: \n" );
			for( i = 0; i < sizeof(char) * SECTORSIZE / sizeof(int); i++ )
			{
				int blockIndex = *((int *) buf + i);
				TracePrintf( level, "%d\n", blockIndex );
				isBlockFree[blockIndex] = USED;
			}
			TracePrintf( level, "\n" );

		}
		return USED;
	}
}

void calculateFreeBlocksAndInodes()
{
	int level = 1000;
	char *where = "yfs.c @ calculateFreeBlocksAndInodes()";
	TracePrintf( level, "[Testing @ %s]: Start:\n", where );

	//Read block 1
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	int readIndirectBlockStatus;
	readIndirectBlockStatus = ReadSector( 1, buf );
	if( readIndirectBlockStatus != 0 )
	{
		TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, 1 );
		return;
	}

	//Get total number of inodes and blocks from fs_header
	struct fs_header *fsHeader = (struct fs_header *) buf;
	int numInodes = fsHeader->num_inodes;
	int numBlocks = fsHeader->num_blocks;
	int *isInodeFree = malloc( sizeof(int) * numInodes );
	int *isBlockFree = malloc( sizeof(int) * numBlocks );

	int i;
	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	for( i = 0; i < numInodes; i++ )
	{
		isInodeFree[i] = FREE;
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks );
	for( i = 0; i < numBlocks; i++ )
	{
		isBlockFree[i] = FREE;
	}
	TracePrintf( level, "\n" );

	//Print inodes in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "BLOCKSIZE/INODESIZE: %d, inodes in %d blocks:\n", BLOCKSIZE / INODESIZE, numOfBlocksContainingInodes );
	int inodeIndex = 0;
	for( i = 1; i < BLOCKSIZE / INODESIZE; i++ )
	{
		int inodeFree = markUsedBlocks( (struct inode *) buf + i, isBlockFree );
		if( inodeFree < 0 )
		{
			TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, inodeIndex );
		}

		isInodeFree[inodeIndex] = inodeFree;
		TracePrintf( level, "[Testing @ %s]: inode %d 's status is %d\n", where, inodeIndex, inodeFree );
		inodeIndex++;
	}
	TracePrintf( level, "\n" );

	//Print inodes not in block 1
	int blockIndex;
	for( blockIndex = 2; blockIndex < numOfBlocksContainingInodes + 1; blockIndex++ )
	{
		readIndirectBlockStatus = ReadSector( blockIndex, buf );
		if( readIndirectBlockStatus != 0 )
		{
			TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, blockIndex );
		}
		else
		{
			for( i = 0; i < BLOCKSIZE / INODESIZE; i++ )
			{
				int inodeFree = markUsedBlocks( (struct inode *) buf + i, isBlockFree );
				if( inodeFree < 0 )
				{
					TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, inodeIndex );
				}
				isInodeFree[inodeIndex] = inodeFree;
				TracePrintf( level, "[Testing @ %s]: inode %d 's status is %d\n", where, inodeIndex, inodeFree );
				inodeIndex++;
			}

			TracePrintf( level, "\n" );
		}
	}

	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	for( i = 0; i < numInodes; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, isInodeFree[i] );
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks );
	for( i = 0; i < numBlocks; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, isBlockFree[i] );
	}
	TracePrintf( level, "\n" );
}

int calculateNumBlocksUsed( int size )
{
	if( size % BLOCKSIZE == 0 )
	{
		TracePrintf(400, "[Testing @ yfs.c @ calculateNumBlocksUsed]: size (%d), blocks used (%d) without remainder\n", size, size/BLOCKSIZE);
		return size / BLOCKSIZE;
	}
	else
	{
		TracePrintf(400, "[Testing @ yfs.c @ calculateNumBlocksUsed]: size (%d), blocks used (%d) with remainder\n", size, size/BLOCKSIZE+1);
		return size / BLOCKSIZE+1;
	}
}

int getBlockNumInodeIn( int inodeNum )
{
	TracePrintf(400, "[Testing @ yfs.c @ getWhichBlockInodeIn]: inode (%d) is in block (%d)\n", inodeNum, inodeNum/(BLOCKSIZE/INODESIZE)+1);
	return inodeNum/(BLOCKSIZE/INODESIZE)+1; 
}

int getInodeIndexWithinBlock(int inodeNum)
{
	int inodeIndex = inodeNum % (BLOCKSIZE/INODESIZE); 
	TracePrintf(400, "[Testing @ yfs.c @ getInodeIndexWithinBlock]: inode (%d)'s index is (%d)\n", inodeNum, inodeIndex);
	return inodeIndex;
}

char* readBlock( int blockNum )
{
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	int readIndirectBlockStatus = ReadSector( blockNum, buf );
	if( readIndirectBlockStatus != 0 )
	{
		TracePrintf( 0, "[Error @ yfs.c @ readBlock]: Read indirect block %d unsuccessfully\n", blockNum );
	}

	return buf;
}

struct inode* readInode( int inodeNum )
{
	int blockNum = getBlockNumInodeIn(inodeNum);

	char *buf = readBlock(blockNum);
	int inodeIndex = getInodeIndexWithinBlock(inodeNum);  
	struct inode *inode = (struct inode *)buf + inodeIndex;
	TracePrintf( 400, "[Testing @ yfs.c  @ readInode]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", inode->type, inode->nlink, inode->size, inode->indirect );
	return inode;
}

//get a list of block number that a file is using
//return num of blocks used
int getUsedBlocks( struct inode *inode, int **usedBlocks )
{
	int level = 400;
	char *where = "yfs.c @ getUsedBlocks";

	//Print basic information about an inode
	TracePrintf( level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode->type, inode->nlink, inode->size, inode->indirect );

	if( inode->type == INODE_FREE )
	{
		TracePrintf( 0, "[Error @ %s @ printInode]: The inode you are trying to get used block is free\n", where );
		return ERROR; //return 0
	}

	int count=0;
	int numBlocks = calculateNumBlocksUsed( inode->size );
	*usedBlocks = (int *) malloc( sizeof(int) * numBlocks );

	if( numBlocks <= NUM_DIRECT )
	{
		//only need to handle direct blocks
		TracePrintf( level, "Direct block: \n" );
		int i;
		for( i = 0; i < numBlocks; i++ )
		{
			TracePrintf( level, "%d\n", inode->direct[i] );
			(*usedBlocks)[count] = inode->direct[i];
			count++;
		}
		TracePrintf( level, "\n" );
	}
	else
	{
		//need to handle indirect blocks
		TracePrintf( level, "Direct block: \n" );
		int i;
		for( i = 0; i < NUM_DIRECT; i++ )
		{
			TracePrintf( level, "%d\n", inode->direct[i] );
			(*usedBlocks)[count] = inode->direct[i];
			count++;
		}
		TracePrintf( level, "\n" );

		//Print indirect block
		if( inode->indirect != 0 )
		{
			char *buf;
			buf = malloc( sizeof(char) * SECTORSIZE );
			int readIndirectBlockStatus;
			readIndirectBlockStatus = ReadSector( inode->indirect, buf );
			if( readIndirectBlockStatus != 0 )
			{
				TracePrintf( 0, "[Error @ %s @ printInode]: Read indirect block %d unsuccessfully\n", where, inode->indirect );
				return ERROR;
			}

			TracePrintf( level, "Indirect block: \n" );
			for( i = 0; i < numBlocks - NUM_DIRECT; i++ )
			{
				int blockIndex = *((int *) buf + i);
				TracePrintf( level, "%d\n", blockIndex );
				(*usedBlocks)[count] = blockIndex;
				count++;
			}
			TracePrintf( level, "\n" );
		}
	}
	return numBlocks;
}

//return the last directory's inode number
int gotoDirectory( char *pathname, int pathNameLen )
{
	int i, j;
	int lastDirectoryInodeNum;
	char c = pathname[0];
	//check is pathname is empty
	if( pathNameLen == 0 || c == '\0' )
	{
		TracePrintf( 0, "[Error @ yfs.c @ gotoDirectory]: pathname's length is 0: %s\n", pathname );
		return ERROR;
	}

	//determine if the file use absolute or relative pathname
	if( c == '/' )
	{
		//This is absolute pathname
		TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: absolute pathname: %s \n", pathname );
		lastDirectoryInodeNum = ROOTINODE;
	}
	else
	{
		//This is relative pathname
		TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: relative pathname: %s \n", pathname );
	}

	//parse the pathname and change the lastDirectoryInodeNum
	for( i = 0; i < MAXPATHNAMELEN; i++ )
	{
		c = pathname[i];
		TracePrintf( 1300, "[Testing @ yfs.c @ gotoDirectory]: char: %c\n", c );

		if( c == '/' )
		{
			if( fileNameCount != 0 )
			{
				TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to this directory (%s) in the current directory\n", fileName );
				fileNameCount = 0;
				for( j = 0; j < DIRNAMELEN; j++ )
				{
					fileName[j] = 0;
				}
			}
		}
		else if( c == '\0' )
		{
			TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: parse char 0\n" );
			break;
		}
		else
		{
			fileName[fileNameCount] = c;
			fileNameCount++;
			if( fileNameCount >= DIRNAMELEN )
			{
				TracePrintf( 0, "[Error @ yfs.c @ gotoDirectory]: file name length exceeds DIRNAMELEN: %s\n", fileName );
			}
		}
	}

	TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to open or create this file (%s) in the current directory\n", fileName );

	return lastDirectoryInodeNum;
}

void readDirectory( int inodeNum )
{

	struct inode *inode = readInode(inodeNum);
	int placeHolder = 0;
	int *usedBlocks = 0;//remember to free this thing somewhere later
	int usedBlocksCount = getUsedBlocks(inode, &usedBlocks);
	TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlockCount: %d\n", usedBlocksCount);
	TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlock: %d\n", usedBlocks);
	int i;
	for(i=0; i<usedBlocksCount; i++)
	{
		TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlock[%d]: %d\n",i, usedBlocks[i]);
	}
	free(usedBlocks);
}

int createFile( char *pathname, int pathNameLen )
{
	int directoryInodeNum = gotoDirectory( pathname, pathNameLen );
	if( directoryInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ Create]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		return ERROR;
	}

	//read directoryInodeNum
	readDirectory(ROOTINODE);
	return 0;
}

void addressMessage( int pid, struct Message *msg )
{
	int type = msg->messageType;
	TracePrintf( 500, "[Testing @ yfs.c @ receiveMessage]: receive message typed %d\n", type );

	int len;
//	char *pathname;
	int size;
	char *buf;
	int inode;
	int currentPos;

	if( type == OPEN || type == CREATE || type == UNLINK || type == MKDIR || type == RMDIR || type == CHDIR || type == STAT || type == READLINK
			|| type == LINK || type == SYMLINK )
	{
		//get pathname and len
		len = msg->len;
//		pathname = malloc(sizeof(char) * len);
		int copyFrom = CopyFrom( pid, pathname, msg->pathname, len );
		if( copyFrom != 0 )
		{
			TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy pathname from pid %d failure\n", pid );
		}
	}

	if( type == READ || type == WRITE || type == LINK || type == SYMLINK || type == READLINK )
	{
		//get buf and size
		size = msg->size;
		buf = malloc( sizeof(char) * size );
		int copyFrom = CopyFrom( pid, buf, msg->buf, size );
		if( copyFrom != 0 )
		{
			TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy buf from pid %d failure\n", pid );
		}
	}

	if( type == READ || type == WRITE )
	{
		//get inode and pos
		inode = msg->inode;
		currentPos = msg->len;
	}

	switch( type )
	{
		case OPEN:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message OPEN: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case CREATE:
			createFile( pathname, len );
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CREATE: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case UNLINK:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message UNLINK: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case MKDIR:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message MKDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case RMDIR:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message RMDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case CHDIR:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CHDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case STAT:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message STAT: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case READLINK:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message READLINK: type(%d), len(%d), pathname(%s), size(%d), buf(%s)\n", type, len,
					pathname, size, buf );
			break;
		case LINK:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message LINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n", type,
					len, pathname, size, buf );
			break;
		case SYMLINK:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYMLINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n",
					type, len, pathname, size, buf );
			break;
		case READ:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message READ: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode,
					currentPos, size, buf );
			break;
		case WRITE:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message WRITE: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode,
					currentPos, size, buf );
			break;
		case SYNC:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYNC: type(%d)\n", type );
			break;
		case SHUTDOWN:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SHUTDOWN: type(%d)\n", type );
			break;
	}
}

int main( int argc, char **argv )
{
	int registerStatus = Register( FILE_SERVER );
	if( registerStatus != 0 )
	{
		TracePrintf( 0, "[Error @ yfs.main]: unsuccessfully register the YFS as the FILE_SERVER.\n" );
	}

//	printDisk( 1500, "main.c" );
	calculateFreeBlocksAndInodes();

	int pid = Fork();
	if( pid == 0 )
	{
		Exec( argv[1], argv + 1 );
	}
	else
	{
		msg = malloc( sizeof(struct Message) );
		pathname = malloc( sizeof(char) * MAXPATHNAMELEN );
		while( 1 )
		{
			TracePrintf( 500, "loop\n" );
			int sender = Receive( msg );
			if( sender == ERROR )
			{
				TracePrintf( 0, "[Error @ yfs.c @ main]: Receive Message Failure\n" );
				return ERROR;
			}

			TracePrintf( 500, "Sender: %d\n", sender );
			addressMessage( sender, msg );
			Reply( msg, sender );
		}
	}

	return 0;
}
