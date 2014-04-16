#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include "global.h"


/* Fucntion prototypes */
void insertIntoLinkedIntList(LinkedIntList* node, LinkedIntList* head, LinkedIntList* tail){
        if(head == 0){ //change to NULL later
                head = node;
                node->next = NULL;
                tail = node;
        }else if( head == tail){
                tail = node;
                head->next = tail;
                node->next = NULL;
        }else{//length >= 3
                tail->next = node;
                tail = node;
        }
}

LinkedIntList* get(int index, LinkedIntList* head){
        LinkedIntList* toReturn = head;
        while(toReturn!=NULL){
                if(toReturn->index == index){
                        return toReturn;
                }
                toReturn = toReturn->next;
        }
        return NULL;
}

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
		TracePrintf( level, "Direct block: \n" );
		int i;
		for( i = 0; i < NUM_DIRECT; i++ )
		{
			TracePrintf( level, "%d\n", inode->direct[i] );
			get(inode->direct[i], isBlockFreeHead)->isFree= USED;
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
				get(blockIndex, isBlockFreeHead)->isFree= USED;
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
/*	int *isInodeFree = malloc( sizeof(int) * numInodes );
	int *isBlockFree = malloc( sizeof(int) * numBlocks );
*/
	LinkedIntList* isInodeFreeHead = NULL;
	LinkedIntList* isBlockFreeHead = NULL;
	LinkedIntList* isInodeFreeTail = NULL; //= malloc( sizeof(LinkedIntList));
	LinkedIntList* isBlockFreeTail = NULL; // = malloc( sizeof(LinkedIntList));

	int i;
	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes);
	for( i = 0; i < numInodes; i++ )
	{
		LinkedIntList* isInodeFree = malloc(sizeof(LinkedIntList));
		isInodeFree->index = i;
		isInodeFree->isFree = FREE;
		insertIntoLinkedIntList(isInodeFree, isInodeFreeHead, isInodeFreeTail);
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks);
	for( i = 0; i < numBlocks; i++ )
	{
		LinkedIntList* isBlockFree = malloc(sizeof(LinkedIntList));
		isBlockFree->index = i;
		isBlockFree->isFree = FREE;
		insertIntoLinkedIntList(isBlockFree, isBlockFreeHead, isBlockFreeTail);
	}
	TracePrintf( level, "\n" );

	//Print inodes in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "BLOCKSIZE/INODESIZE: %d, inodes in %d blocks:\n", BLOCKSIZE/INODESIZE, numOfBlocksContainingInodes );
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

	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes);
	for( i = 0; i < numInodes; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, get(i,isInodeFreeHead)->isFree );
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks);
	for( i = 0; i < numBlocks; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, get(i,isBlockFreeHead)->isFree );
	}
	TracePrintf( level, "\n" );
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

	Fork();
	Exec(argv[1], argv+1);

//	while(1)
//	{
//		char *msg = malloc(sizeof(char) * 32);
//		int receive = Receive(msg);
//		if(receive != 0)
//		{
//			TracePrintf(0, "[Error @ yfs.c @ main]: Receive Message Failure\n");
//			return ERROR;
//		}
//	}
	
	return 0;
}
