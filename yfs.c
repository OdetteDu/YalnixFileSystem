#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include "global.h"

/* Global Variables */
struct Message *msg;
char *pathname;
//char fileName[DIRNAMELEN];
//int fileNameCount;


//the working directory inode number is the ROOTINODE at the 
// beginning of things....
int workingDirectoryInodeNumber = ROOTINODE;
/* Initialize the linkedIntList */
LinkedIntList* isInodeFreeHead=NULL;
LinkedIntList* isBlockFreeHead=NULL;
LinkedIntList* isInodeFreeTail=NULL; //= malloc( sizeof(LinkedIntList));
LinkedIntList* isBlockFreeTail=NULL; // = malloc( sizeof(LinkedIntList));


/* Function declarations */
//for general linked list
void insertIntoLinkedIntList(LinkedIntList *node, LinkedIntList **head, LinkedIntList **tail);
LinkedIntList* get(int index, LinkedIntList *head);
void printLinkedList(int level, char *where, LinkedIntList* head);

//for inode and block
int getFreeInode();

int readDirectory( int inodeNum, char *filename, int fileNameLen );
//end of function declarations


/* Implementations */

int getFreeInode(){
	LinkedIntList * node = isInodeFreeHead;
	while(node != NULL){
		  if(node->isFree == FREE){
			  node->isFree = USED;
			  return node->index;
		  }
		  node = node->next;
	}
	return -1;
}
//begin of linked list methods
void insertIntoLinkedIntList(LinkedIntList* node, LinkedIntList** head, LinkedIntList** tail){
        if(*head == NULL){ //change to NULL later
                *head = node;
                node->next = NULL;
                *tail = node;
        }else if( *head == *tail){
                *tail = node;
                (*head)->next = *tail;
                node->next = NULL;
        }else{//length >= 3
                (*tail)->next = node;
                *tail = node;
        }
}

LinkedIntList* get(int index, LinkedIntList* head){
		LinkedIntList* toReturn= head;
        while(toReturn!=NULL){
                if(toReturn->index == index){
                        return toReturn;
                }
                toReturn = toReturn->next;
        }
        return NULL;
}

void printLinkedList(int level, char *where, LinkedIntList* head){
	TracePrintf(level, "[Testing @ %s]: Printing LinkedIntList*:\n", where);
	LinkedIntList* toPrint = head;
	while(toPrint != NULL){
		  TracePrintf(level, "[Testing @ %s]: LinkedIntListNode at %p, next at %p, isFree(%d), index(%d)\n", toPrint, toPrint->next, toPrint->isFree, toPrint->index);
		  toPrint = toPrint->next;
	}

}
//end of linked list methods


/* Utility functions for printing fs */
void printInode( int level, char *where, struct inode *inode )
{
	//Print basic information about an inode
	TracePrintf( level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode->type, inode->nlink, inode->size,
			inode->indirect );

	//Print direct block
	TracePrintf( level, "prDirect block: \n" );
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
	TracePrintf( level, "[Testing @ %s @ printDisk] fs_header: num_blocks(%d), num_inodes(%d)\n",where, fsHeader->num_blocks, fsHeader->num_inodes );

	//Print inodesi in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "[Testing @ %s @ printDisk] inodes in %d blocks:\n",where, numOfBlocksContainingInodes );
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
	free(buf);
	TracePrintf( level, "\n" );
}
/* End of utility functions for printing */

/* Begin helper methods for yfs server io calls */
//mark used blocks used by inode
int markUsedBlocks( struct inode *inode, LinkedIntList *isBlockFreeHead )
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
		TracePrintf( level, "[Testing @ %s] Direct block: \n", where );
		int i;
		for( i = 0; i < NUM_DIRECT; i++ )
		{
			TracePrintf( level, "[Testing @ %s] At inode(%p)->direct[%d], %d\n", where, inode, i, inode->direct[i] );
			LinkedIntList* block = get(inode->direct[i], isBlockFreeHead);//->isFree= USED;
			TracePrintf(level, "[Testing @ %s]: direct[%d] ptr: %p\n", where, i, block);
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

			TracePrintf( level, "[Testing @ %s] Indirect block: \n", where );
			for( i = 0; i < sizeof(char) * SECTORSIZE / sizeof(int); i++ )
			{
				int blockIndex = *((int *) buf + i);
				TracePrintf( level, "[Testing @ %s] indirect block [%d] index: %d\n", where, i, blockIndex );
				get(blockIndex, isBlockFreeHead)->isFree= USED;
			}
			TracePrintf( level, "\n" );
			free(buf);
		}
		return USED;
	}
}

//this is used for initilizing the file system
void calculateFreeBlocksAndInodes()
{
	int level = 1000;
	char *where = "yfs.c @ calculateFreeBlocksAndInodes()";
	TracePrintf( level, "[Testing @ %s]: Start:\n", where );

	//Read block 1
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	if(buf == NULL) {
		   TracePrintf(level, "[Testing @ %s]: malloc failure for buf\n", where);
	}
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
	
	int i;
	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	for( i = 0; i < numInodes; i++ )
	{
		LinkedIntList* isInodeFree = malloc(sizeof(LinkedIntList));
		isInodeFree->index = i;
		isInodeFree->isFree = FREE;
		insertIntoLinkedIntList(isInodeFree, &isInodeFreeHead, &isInodeFreeTail);
	}
	TracePrintf( level, "\n" );
	
	printLinkedList(level, where, isInodeFreeHead);

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks );
	for( i = 0; i < numBlocks; i++ )
	{
		LinkedIntList* isBlockFree = malloc(sizeof(LinkedIntList));
		if(isBlockFree == NULL){
			  //failed to malloc node on heap
			  TracePrintf(level, "[Testing @ %s]: Marking free blocks, failed to malloc at %d\n",
					  where, i);
		}
		isBlockFree->index = i;
		isBlockFree->isFree = FREE;
		insertIntoLinkedIntList(isBlockFree, &isBlockFreeHead, &isBlockFreeTail);
		//TracePrintf(level, "[Testing @ %s]: Marked Block %d free:\n", where, i);
	}
	TracePrintf( level, "\n" );
	
	printLinkedList(level, where, isBlockFreeHead);
	//Print inodes in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "[Testing @ %s] BLOCKSIZE/INODESIZE: %d, inodes in %d blocks:\n", where, BLOCKSIZE / INODESIZE, numOfBlocksContainingInodes );
	int inodeIndex = 0;
	for( i = 1; i < BLOCKSIZE / INODESIZE; i++ )
	{
		int inodeFree = markUsedBlocks( (struct inode *) buf + i, isBlockFreeHead );
		if( inodeFree < 0 )
		{
			TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, inodeIndex );
		}

		get(inodeIndex, isInodeFreeHead)->isFree = inodeFree;
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
				int inodeFree = markUsedBlocks( (struct inode *) buf + i, isBlockFreeHead );
				if( inodeFree < 0 )
				{
					TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, inodeIndex );
				}
				get(inodeIndex, isInodeFreeHead)->isFree = inodeFree;
				TracePrintf( level, "[Testing @ %s]: inode %d 's status is %d\n", where, inodeIndex, inodeFree );
				inodeIndex++;
			}

			TracePrintf( level, "\n" );
		}
	}

	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	for( i = 0; i < numInodes; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, get(i,isInodeFreeHead)->isFree );
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks );
	for( i = 0; i < numBlocks; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, get(i,isBlockFreeHead)->isFree );
	}
	TracePrintf( level, "\n" );

	/* Free the malloced item */
	free(buf);
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

/* Given an inodeNumber return a block number */
int getBlockNumInodeIn( int inodeNum )
{
	TracePrintf(400, "[Testing @ yfs.c @ getWhichBlockInodeIn]: inode (%d) is in block (%d)\n", inodeNum, inodeNum/(BLOCKSIZE/INODESIZE)+1);
	return inodeNum/(BLOCKSIZE/INODESIZE)+1; 
}

/* GIven an inodeNumber return a inode index in the block */
int getInodeIndexWithinBlock(int inodeNum)
{
	int inodeIndex = inodeNum % (BLOCKSIZE/INODESIZE); 
	TracePrintf(400, "[Testing @ yfs.c @ getInodeIndexWithinBlock]: inode (%d)'s index is (%d)\n", inodeNum, inodeIndex);
	return inodeIndex;
}

/* Read and write for block */
char* readBlock( int blockNum )
{
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	int readBlockStatus = ReadSector( blockNum, buf );
	if( readBlockStatus != 0 )
	{
		TracePrintf( 0, "[Error @ yfs.c @ readBlock]: Read block %d unsuccessfully\n", blockNum );
	}
	//TODO: free buf at any call to readBlock after use
	return buf;
}

int writeBlock( int blockNum, char *buf)
{
	int writeBlockStatus = WriteSector(blockNum, buf);
	if(writeBlockStatus != 0)
	{
		TracePrintf( 0, "[Error @ yfs.c @ writeBlock]: Write block %d unsuccessfully\n", blockNum );
		return ERROR;
	}
	TracePrintf( 350, "[Testing @ yfs.c @ writeBlock]: Write block %d successfully\n", blockNum );
	return 0;
}

/* Read and write for inode */
struct inode* readInode( int inodeNum )
{
	int blockNum = getBlockNumInodeIn(inodeNum);

	char *buf = readBlock(blockNum);
	int inodeIndex = getInodeIndexWithinBlock(inodeNum);  
	//struct inode* inode = malloc(sizeof(struct inode));
	//TODO: should do some mem copy instead of directly using buf here
	// THIS WILL CAUSE MEMORY LEAK
	// NEED TO FIX LATER
	// malloc inode
	// free(buf)
	// memcpy(inode, buf, blocksize - inodeindex);
	//memcpy((struct inode *)buf + inodeIndex, inode, sizeof(struct inode*));
//	free(buf);
	//int returnStatus = 0;

	struct inode *inode = malloc(sizeof(struct inode *));
	memcpy(inode, (struct inode *)buf + inodeIndex, sizeof(struct inode*));
	TracePrintf( 400, "[Testing @ yfs.c  @ readInode]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", inode->type, inode->nlink, inode->size, inode->indirect );
	free(buf);//TODO: THIS FREE SEEMS UNSAFE!!
	return inode;
}

struct inode* writeInode( int inodeNum, struct inode *newInode )
{
	int blockNum = getBlockNumInodeIn(inodeNum);

	char *buf = readBlock(blockNum);
	int inodeIndex = getInodeIndexWithinBlock(inodeNum);  
	memcpy((struct inode *)buf + inodeIndex, newInode, sizeof(struct inode *));
	struct inode *inode = newInode;
	TracePrintf( 400, "[Testing @ yfs.c  @ writeInode]: inode after write(type: %d, nlink: %d, size: %d, indirect: %d)\n", inode->type, inode->nlink, inode->size, inode->indirect );
	writeBlock(blockNum, buf);
//TODO: CHECK MEMROY HERE
	free(buf);
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
		TracePrintf( level, "Indirect block: \n" );
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
//This method also changes fileName and fileNameCount
int gotoDirectory( char *pathname, int pathNameLen, int* lastExistingDir, int * fileNameCount, char** fileName)
{
	int i;
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
		//change it to working directory inode number
		lastDirectoryInodeNum = workingDirectoryInodeNumber;

		TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: relative pathname: %s \n", pathname );
	}

	(*lastExistingDir) = lastDirectoryInodeNum;
	TracePrintf(300, "[Testing @ yfs.c @ gotoDirectory]: start to parse path name\n");
	char prev = 0;
	//parse the pathname and change the lastDirectoryInodeNum
	for( i = 0; i < MAXPATHNAMELEN; i++ )
	{
//		TracePrintf(500, "[Testing @ yfs.c @ gotoDirectory]: reading c\n");
		c = pathname[i];
		TracePrintf( 500, "[Testing @ yfs.c @ gotoDirectory]: char: %c\n", c );

		if( c == '/')
		{
			if( *fileNameCount != 0 )
			{
				TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to this directory (%s) in the current directory\n", *fileName );
				
				//Change the lastExistingDir inumber
				(*lastExistingDir) = readDirectory(*lastExistingDir, *fileName, *fileNameCount);
				if(*lastExistingDir == 0){
					TracePrintf(300, "[Error @ yfs.c @ gotoDirectory]: path contains : (%s), it is not a directory\n",*fileName);  
					return ERROR;
				}
				*fileNameCount = 0;
				memset(*fileName, '\0', DIRNAMELEN);
			
			}
		}
		else if( c == '\0' )
		{
			TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: parse char 0\n" );
			break;
		}
		else
		{
//			TracePrintf( 500, "[Testing @ yfs.c @ gotoDirectory]: writing c\n");
			(*fileName)[*fileNameCount]= c;
			(*fileNameCount)++;
			if( *fileNameCount >= DIRNAMELEN )
			{
				TracePrintf( 0, "[Error @ yfs.c @ gotoDirectory]: file name length exceeds DIRNAMELEN:(%s)\n", *fileName );
			return ERROR;
			}
		}

		prev = c;
	}

	TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to open or create this file (%s) in the current directory\n", *fileName );

	return lastDirectoryInodeNum;
}

int readDirectory( int inodeNum, char *filename, int fileNameLen )
{
	//need to check if read inode successfully, such as make sure inodeNum is in correct bound
	struct inode *inode = readInode(inodeNum);
	printInode(inodeNum, "readDirectory", inode);
	int *usedBlocks = malloc(sizeof(int));//remember to free this thing somewhere later

	//need to check if get used blocks successfully, the inode may be a free inode
	int usedBlocksCount = getUsedBlocks(inode, &usedBlocks);
	TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlockCount: %d\n", usedBlocksCount);
	TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlock: %d\n", usedBlocks);
	int numDirEntries = (inode -> size)/sizeof(struct dir_entry);
	int directoryCount = 0;
	int index = 0;
	int i;
	for(i=0; i<usedBlocksCount; i++)
	{
		TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: usedBlock[%d]: %d\n",i, usedBlocks[i]);
		char *buf = readBlock(usedBlocks[i]);
		while(index < BLOCKSIZE / sizeof(struct dir_entry) && directoryCount < numDirEntries)
		{
			struct dir_entry *dirEntry = (struct dir_entry *)buf + index;
			TracePrintf(300, "[Testing @ yfs.c @ readDirectory]: block (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", i, index, directoryCount, dirEntry -> inum, dirEntry -> name);

			//compare the dir_entry's name and fileName
			int compare = strncmp(dirEntry -> name, filename, DIRNAMELEN);
			if(compare == 0)
			{
				//match found
				return dirEntry -> inum;
			}

			index ++;
			directoryCount ++;
		}
		index = 0;
	}
	free(usedBlocks);
	return 0;//meaning unfound
}

int writeNewEntryToDirectory( int inodeNum, struct dir_entry *newDirEntry )
{
	//need to check if read inode successfully, such as make sure inodeNum is in correct bound
	struct inode *inode = readInode(inodeNum);

	int dirEntryIndex = ((inode -> size) % BLOCKSIZE) / sizeof(struct dir_entry);

	if(dirEntryIndex == 0)
	{
		//need to allocate new data block to store the dir data
		TracePrintf(350, "[Testing @ yfs.c @ writeNewEntryToDirectory]: need to allocate new data block to store the new dir data\n");
		//TODO
	}
	else
	{
		//can use the same data block to store the dir data
		TracePrintf(350, "[Testing @ yfs.c @ writeNewEntryToDirectory]: can use the same data block to store the new dir data\n");
		int *usedBlocks = 0;//remember to free this thing somewhere later
		//need to check if get used blocks successfully, the inode may be a free inode
		int usedBlocksCount = getUsedBlocks(inode, &usedBlocks);

		int blockNum = usedBlocks[usedBlocksCount-1];
		TracePrintf(300, "[Testing @ yfs.c @ writeNewEntryToDirectory]: usedBlockCount: %d, blockNum tobe write: %d\n", usedBlocksCount, blockNum);
		char *buf = readBlock(blockNum);
		memcpy((struct dir_entry *)buf + dirEntryIndex, newDirEntry, sizeof(struct dir_entry *));
		struct dir_entry *dirEntry = (struct dir_entry *)buf + dirEntryIndex;
		TracePrintf(300, "[Testing @ yfs.c @ writeNewEntryToDirectory]: block (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", blockNum, dirEntryIndex, dirEntryIndex, dirEntry -> inum, dirEntry -> name);
		writeBlock(blockNum, buf);
		free(usedBlocks);

		struct inode *inode = readInode(inodeNum);
		int originalSize = inode -> size;
		int newSize = originalSize + sizeof(struct dir_entry);
		inode -> size = newSize;
		TracePrintf(300, "Testin @ yfs.c @ writeNewEntryToDirectory]: originalSize: %d, newSize: %d\n", originalSize, newSize);
		writeInode(inodeNum, inode);	
	}
	//free
	return 0;
}


/* YFS Server calls corresponding to the iolib calls */
int createFile( char *pathname, int pathNameLen )
{
	int lastExistingDir;
	char *fileName = malloc(sizeof(char)*DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	int fileNameCount;
	
	int directoryInodeNum = gotoDirectory( pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName );
	if( directoryInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ CreateFile]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		return ERROR;
	}

	//read directoryInodeNum
	//	int fileInodeNum = readDirectory(workingDirectoryInodeNumber, fileName, fileNameCount);//TODO, not always ROOTINODE
	
	int fileInodeNum = readDirectory(lastExistingDir, fileName,fileNameCount);//TODO, not always ROOTINODE
	if(fileInodeNum == 0)
	{
		//file not found, make a new file
		//create a newInode for the new file
		int newInodeNum = getFreeInode(); //allocate inode num 
		struct inode *inode = readInode(newInodeNum);
		inode -> type = INODE_REGULAR;
		inode -> nlink = 1; 
		inode -> size = 0;
		writeInode(newInodeNum, inode);	

		//create a new dir_entry for the new file
		struct dir_entry *newDirEntry;
		newDirEntry = malloc(sizeof(struct dir_entry));
		newDirEntry -> inum = newInodeNum;

		//this part may be substituted by a c libaray function
		memcpy(newDirEntry->name, fileName, DIRNAMELEN);

		TracePrintf( 0, "[Testing @ yfs.c @ CreateFile]: new dir_entry created: inum(%d), name(%s)\n", newDirEntry -> inum, newDirEntry -> name );
		writeNewEntryToDirectory(lastExistingDir, newDirEntry);//TODO, not always ROOTINODE
	}
	else
	{
		//TODO: Test this part
		//file found, empty the file
		//only tuncate if this file is INODE_REGULAR
		struct inode *inode = readInode(fileInodeNum);
		if(inode->type != INODE_REGULAR){
			TracePrintf(0, "[Error @ yfs.c @ CreateFile]: file %s exists and is not writable\n", fileName);

			return ERROR;
		}
		//TODO: free all the used blocks!!
		
	}

	//TODO: clean up fileName and fileNameCount
	fileNameCount = 0;
	free(fileName);
//	fileInodeNum = readDirectory(ROOTINODE, fileName, fileNameCount);//test purpose only, should be delete
	return fileInodeNum;
}

//Open file or a directory

int openFileOrDir(char *pathname, int pathNameLen){
	int lastExistingDir;
	int fileNameCount=0;
	char *fileName = malloc(sizeof(char)*DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	//fill in fileName and fileName count;
	int directoryInodeNum = gotoDirectory( pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName);
	if( directoryInodeNum == ERROR )
	{
			TracePrintf( 0, "[Error @ yfs.c @ CreateFile]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		return ERROR;
	  
	}
	
	int fileInodeNum = readDirectory(directoryInodeNum, fileName, fileNameCount);

	return fileInodeNum; 
}
// Make a directory

int mkDir(char * pathname, int pathNameLen){
	  
	TracePrintf(0, "[Testing @ yfs.c @ mkDir] Entering Mkdir, requesting %s\n", pathname);
	int fileNameCount;
	char *fileName = malloc(sizeof(char)*DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	
	int lastExistingDir;
	int directoryInodeNum = gotoDirectory(pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName);

	if(directoryInodeNum  == ERROR){
			TracePrintf(0, "[Error @ yfs.c @ mkDir]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		return ERROR;
	}

	TracePrintf(0, "[Testing @ yfs.c @ mkDir]: try to make directory full path: %s, actual path needs to be created: %s\n", pathname, fileName);
		//fileName is the directory we need to create:
	
	//int fileInodeNum = readDirectory(lastExistingDir, fileName, fileNameCount);	
	if(fileNameCount == 0)
	{
		//This means the directory already exists
		TracePrintf(0, "[Error @ yfs.c @ mkDir]: Trying to create a directory that already exists, pathname(%s)\n", pathname);
		return ERROR;
	}
	else
	{	
		TracePrintf(0, "[Testing @ yfs.c @ mkDir]: try to make directory full path: %s, actual path needs to be created: %s\n", pathname, fileName);
		//create a new directory with name 
	
		int fileInodeNum = readDirectory(lastExistingDir, fileName, fileNameCount);	
		if(fileInodeNum == 0){
			//file does not exist, create a new one
			int newInodeNum = getFreeInode(); //allocate inode num 
			struct inode *inode = readInode(newInodeNum);
			inode -> type = INODE_DIRECTORY;
			inode -> nlink =2; 
			inode -> size = 0;
			writeInode(newInodeNum, inode);	
			//create a new dir_entry for the new file
			struct dir_entry *newDirEntry;
			newDirEntry = malloc(sizeof(struct dir_entry));
			newDirEntry -> inum = newInodeNum;

			//copy
			memcpy(newDirEntry->name, fileName, DIRNAMELEN);
				
			TracePrintf( 0, "[Testing @ yfs.c @ mkDIr]: new dir_entry created: inum(%d), name(%s)\n", newDirEntry -> inum, newDirEntry -> name );
			/* Add this directory entry to lastExistngDir which is the parent */
			writeNewEntryToDirectory(lastExistingDir, newDirEntry);//TODO, not always ROOTINODE
			free(newDirEntry);

			/* Add . and .. into the new directory */
			
		//	writeInode(newInodeNum, inode);
			newDirEntry = malloc(sizeof(struct dir_entry));
			memset(newDirEntry->name, '\0', DIRNAMELEN);
			memcpy(newDirEntry->name, ".", 1);
			TracePrintf(0, "[Testing @ yfs.c @ mkDir]: added . to the dir we create at %s \n", fileName);
			writeNewEntryToDirectory(newInodeNum, newDirEntry);
			free(newDirEntry);

			newDirEntry = malloc(sizeof(struct dir_entry));
			memset(newDirEntry->name, '\0', DIRNAMELEN);
			memcpy(newDirEntry->name, "..", 2);
			TracePrintf(0, "[Testing @ yfs.c @ mkDir]: added .. to the dir we create at %s \n", fileName);
			writeNewEntryToDirectory(newInodeNum, newDirEntry);
			free(newDirEntry);
			free(fileName);
			return  0;
		}else{
			TracePrintf(0, "[Error @ yfs.c @ mkDir]: try to make a direcotry path: %s, but already exists\n", pathname);
			return  ERROR;
		}
	}

	fileNameCount = 0;
	//memset(fileName, '\0', DIRNAMELEN);
	free(fileName);
	return 0;

}

// Change to a new dir

int chDir(char *pathname, int pathNameLen){
	int fileNameCount;
	char *fileName = malloc(sizeof(char)*DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	
	 TracePrintf(0, "[Testing @ yfs.c @ chDir] Entering Chdir, requesting %s\n", pathname);
	int lastExistingDir;
	int directoryInodeNum = gotoDirectory(pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName);
	
	if(directoryInodeNum  == ERROR){
			TracePrintf(0, "[Error @ yfs.c @ chDir]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		return ERROR;
			  
	}
	
	if(fileNameCount == 0){
		  //pathname is the directory we want to go to 
		  TracePrintf(0, "[Testing @ yfs.c @ chDir]: found directory : %s\n", pathname);
		  //TODO:get the inode from inodenumber to verify INODE_DIRECTORY
		  workingDirectoryInodeNumber = lastExistingDir;
		  //changed working directory number;
		  return 0;	
	}else{
		  
		//try to goto fileName as a directory;
		int fileInodeNumber = readDirectory(lastExistingDir, fileName, fileNameCount);
		if(fileInodeNumber == 0){
			TracePrintf(0, "[Error @ yfs.c @ chDir]: file not found: %s, full path reqeusted: %s\n", fileName, pathname);
			return ERROR;
		}

		//get the inode and check type
		struct inode * fileInode= readInode(fileInodeNumber);
		if(fileInode->type != INODE_DIRECTORY){
			  TracePrintf(0, "[Error @ yfs.c @ chDir] this pathname is not a directory:%s, full path requested: %s\n", fileName, pathname);
			  return ERROR;
		}
		workingDirectoryInodeNumber = fileInodeNumber;
		return 0;
	}
	//when fileNameCount != 0, 
	TracePrintf(0, "[Error @ yfs.c @ chDir]: path not found (%s), full name(%s)\n", fileName,pathname);
	fileNameCount = 0;
	free(fileName);
	//memset(fileName, 0, DIRNAMELEN);
	return ERROR;

}

//remove a directory
int rmDir(char* pathname, int pathNameLen){
	int lastExistingDir;
	int fileNameCount;
	char *fileName = malloc(sizeof(char)*DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	memset(fileName, '\0', DIRNAMELEN);
	
	int directoryInodeNum = gotoDirectory(pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName);
	if(directoryInodeNum == ERROR){
		  TracePrintf(0, "[Error @ yfs.c @ rmDir]: directoryInodeNum is Error: pathname: %s, filename: %s\n", pathname, fileName);
		  return ERROR;
	}

	
		//just remove lastExisting dir
		struct inode * temp;
		if(fileNameCount==0){
			temp = readInode(lastExistingDir);//TODO: free this
			
		}else{
			int dir = readDirectory(lastExistingDir, fileName, fileNameCount);
			if(dir == 0 ){
				  TracePrintf(0, "[Error @ yfs.c @ rmDir]: non existing dir requested : (%s)\n", pathname);
				  return ERROR;
			}
			temp = readInode(dir);
			if(temp->type != INODE_DIRECTORY){
				TracePrintf(0, "[Error @ yfs.c @ rmDir]: try to remove non directory: (%s)\n", pathname);  
				return ERROR;
			}
			
		}
		temp->type = INODE_FREE;
		if(temp->indirect != 0){
			  //TODO: free all the indirect inodes
		}
		int i;
		for(i=0; i<NUM_DIRECT; i++){
			  if(temp->direct[i] == 0){
				break;
			  }
			  //TODO: mark direct[i] block free
			  temp->direct[i]=0;
		}
		writeInode(lastExistingDir, temp);
//TODO: mark temp as a free in the free list
	free(fileName);
	return 0;

}


int yfsRead(int inode){
	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;
}

int yfsWrite(int inode){
	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  

}

int link(struct Message * msg){
		TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  
  

}

int unlink(char * pathname, int pathnamelen){
	  	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  


}

int symLink(struct Message *msg){
	  	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  


}

int readLink(struct Message *msg){
	  	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  


}

int stat(struct Message *msg){
	  	TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  


}

int sync(){
	  
		TracePrintf(0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n");	  
	return 0;	  
//SYNC THE CACHE
}
/* End of yfs server calls */

void addressMessage( int pid, struct Message *msg )
{
	int type = msg->messageType;
	TracePrintf( 500, "[Testing @ yfs.c @ receiveMessage]: receive message typed %d\n", type );

	int len;
	char *pathname;
	int size;
	char *buf;
	int inode;
	int currentPos;

	if( type == OPEN || type == CREATE || type == UNLINK || type == MKDIR || type == RMDIR || type == CHDIR || type == STAT || type == READLINK
			|| type == LINK || type == SYMLINK )
	{
		//get pathname and len
		len = msg->len;
		pathname = malloc(sizeof(char) * len);
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
		
			//TODO:OPEN
			msg->inode = openFileOrDir(pathname, len);
			break;
		case CREATE:
			msg->inode = createFile( pathname, len );
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CREATE: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case UNLINK:
			//TODO: UNLINK
			//
			unlink(pathname, len);
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message UNLINK: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case MKDIR:
			//TODO: MIDIR
			msg->returnStatus = mkDir(pathname, len);
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message MKDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case RMDIR:
			//TODO: RMDIRi
			msg->returnStatus=rmDir(pathname, len);
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message RMDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case CHDIR:
			//TODO: CHDIR
		
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CHDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			msg->returnStatus = chDir(pathname, len);
			break;
		case STAT:
			//TODO: STAT
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message STAT: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			break;
		case READLINK:
			//TODO: READLINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message READLINK: type(%d), len(%d), pathname(%s), size(%d), buf(%s)\n", type, len,
					pathname, size, buf );
			break;
		case LINK:
			//TODO: LINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message LINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n", type,
					len, pathname, size, buf );
			break;
		case SYMLINK:
			//TODOZ: SYMLINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYMLINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n",
					type, len, pathname, size, buf );
			break;
		case READ:
			//TODO: READ
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message READ: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode,
					currentPos, size, buf );
			break;
		case WRITE:
			//TODO: WRITE
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message WRITE: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode,
					currentPos, size, buf );
			break;
		case SYNC:
			//TODO: SYNC
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYNC: type(%d)\n", type );
			break;
		case SHUTDOWN:
			//TODO: SHUTDOWN
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SHUTDOWN: type(%d)\n", type );
			break;
	}

//	free(msg);
}

int main( int argc, char **argv )
{
	int registerStatus = Register( FILE_SERVER );
	if( registerStatus != 0 )
	{
		TracePrintf( 0, "[Error @ yfs.main]: unsuccessfully register the YFS as the FILE_SERVER.\n" );
	}
	//set the current workingDirectoryInodeNumber to be ROOTINODE
//	workingDirectoryInodeNumber = ROOTINODE;
//	printDisk( 1500, "main.c" );
	calculateFreeBlocksAndInodes();
	

	//set all char in fileName \0, 
	//TODO: check understanding
//	memset(fileName, '\0', DIRNAMELEN);

	/*
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
	*/

	return 0;
}
