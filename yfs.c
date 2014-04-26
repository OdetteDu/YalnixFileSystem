#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>
#include "global.h"

/* Global Variables */
int numInodes;
int numBlocks;

LinkedIntList* isInodeFreeHead = NULL;
LinkedIntList* isBlockFreeHead = NULL;
LinkedIntList* isInodeFreeTail = NULL; //= malloc( sizeof(LinkedIntList));
LinkedIntList* isBlockFreeTail = NULL; // = malloc( sizeof(LinkedIntList));

struct CacheBlockNode
{
	int blockNum;
	int isDirty;
	char data[BLOCKSIZE];
	struct CacheBlockNode *HashPrev;
	struct CacheBlockNode *HashNext;
	struct CacheBlockNode *LRUPrev;
	struct CacheBlockNode *LRUNext;
};

int numCachedBlock = 0;
struct CacheBlockNode *blockCacheTable[BLOCK_CACHESIZE];
struct CacheBlockNode *blockLRUHead = NULL;

char* readBlock( int );
int writeBlock( int, char *);

void printLRUBlockCache()
{
	struct CacheBlockNode *current = blockLRUHead;

	while((current -> LRUNext) != blockLRUHead)
	{
		TracePrintf(100, "Testing @ yfs.c @ printDoubleLinkedList]: LRU Print: %d, prev(%d), next(%d)\n", current -> blockNum, (current -> LRUPrev)->blockNum, (current -> LRUNext) -> blockNum);    
		current = current -> LRUNext;
	}
	TracePrintf(100, "Testing @ yfs.c @ printDoubleLinkedList]: LRU Print: %d, prev(%d), next(%d)\n", current -> blockNum, (current -> LRUPrev)->blockNum, (current -> LRUNext) -> blockNum);    
}

void printHashBlockCache()
{
	int i;
	for(i=0; i<BLOCK_CACHESIZE; i++)
	{
		struct CacheBlockNode *current = blockCacheTable[i];
	
		if(current != NULL)
		{
			while((current -> HashNext) != blockCacheTable[i])
			{
				TracePrintf(100, "Testing @ yfs.c @ printDoubleLinkedList]: Hash Print(%d): %d, prev(%d), next(%d)\n", i, current -> blockNum, (current -> HashPrev)->blockNum, (current -> HashNext) -> blockNum);    
				current = current -> HashNext;
			}
			TracePrintf(100, "Testing @ yfs.c @ printDoubleLinkedList]: Hash Print(%d): %d, prev(%d), next(%d)\n", i, current -> blockNum, (current -> HashPrev)->blockNum, (current -> HashNext) -> blockNum);    
		}
		
	}
}

int HashFunc( int num )
{
	//Perform the hash Function
	int index = num % BLOCK_CACHESIZE;
	TracePrintf( 100, "[Testing @ yfs.c @ HashFunc]: Hash Func: num(%d), index(%d)\n", num, index );
	return index;
}

struct CacheBlockNode *getBlockFromCache( int blockNum )
{
	TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromHash]: Start: blockNum(%d)\n", blockNum );

	int index = HashFunc( blockNum );

	struct CacheBlockNode *cacheNode = blockCacheTable[index];
	while( cacheNode != NULL && cacheNode->blockNum != blockNum )
	{
		TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromHash]: Looking for blockNum(%d), current blockNum(%d)\n", blockNum, cacheNode->blockNum );
		cacheNode = cacheNode->HashNext;
		if(cacheNode == blockCacheTable[index])
		{
			cacheNode = NULL;
		}
	}

	if( cacheNode == NULL )
	{
		TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: The block(%d) is not in cache yet, need to load from the disk\n", blockNum );

		//Determine if necessary to free a slot in cache to store a new blockNum
		if( numCachedBlock >= BLOCK_CACHESIZE )
		{
			//The cache is full, need to free a slot to load more 
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: the block cache is full, need to free a slot, current: %d, limit: %d\n", numCachedBlock, BLOCK_CACHESIZE );

			//Get and remove the Head in the LRU
			struct CacheBlockNode *tobeRemove = blockLRUHead;
			struct CacheBlockNode *prev = blockLRUHead -> LRUPrev;
			struct CacheBlockNode *next = blockLRUHead -> LRUNext;
			prev -> LRUNext = next;
			next -> LRUPrev = prev;
			blockLRUHead = next;

			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU After Remove: Head: %d, prev(%d), next(%d), Current: %d, prev(%d), next(%d), Prev: %d, prev(%d), next(%d), Next: %d, prev(%d), next(%d)\n", blockLRUHead -> blockNum, (blockLRUHead -> LRUPrev) -> blockNum, (blockLRUHead -> LRUNext) -> blockNum,tobeRemove->blockNum, (tobeRemove->LRUPrev)->blockNum, (tobeRemove->LRUNext)->blockNum,prev->blockNum, (prev->LRUPrev)->blockNum, (prev->LRUNext)->blockNum,next->blockNum, (next->LRUPrev)->blockNum, (next->LRUNext)->blockNum );

			printLRUBlockCache();
			
			//remove it from Hash
			int tobeRemoveIndex = HashFunc(tobeRemove -> blockNum);
			prev = tobeRemove -> HashPrev;
			next = tobeRemove -> HashNext;
			prev -> HashNext = next;
			next -> HashPrev = prev;
			if(blockCacheTable[tobeRemoveIndex] == cacheNode)
			{
				TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: HashHead == cacheNode\n"); 
				if(tobeRemove == prev && tobeRemove == next)
				{
					TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: HashHead is the last cacheNode\n"); 
					blockCacheTable[tobeRemoveIndex] = NULL; 
				}
				else
				{
					TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: HashHead is not the last cacheNode\n"); 
					blockCacheTable[tobeRemoveIndex] = next; 
				}
			}
			printHashBlockCache();

			//if it is dirty, write to disk, then free the node
			if((tobeRemove -> isDirty) == 1)
			{
				char *data = malloc(sizeof(char) * BLOCKSIZE);
				memcpy(data, tobeRemove -> data, BLOCKSIZE);
				writeBlock(tobeRemove -> blockNum, data);
			}
			free(tobeRemove);
		}

		//Load the data inoto cache
		char *data = readBlock( blockNum );
		TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: Get Data: blockNum(%d), data(%s)\n", blockNum, data );
		cacheNode = malloc( sizeof(struct CacheBlockNode) );
		cacheNode->blockNum = blockNum;
		cacheNode->isDirty = 0;
		memcpy( &(cacheNode->data), data, BLOCKSIZE );
		TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: New CacheNode: blockNum(%d), isDirty(%d), data(%s)\n", cacheNode->blockNum, cacheNode->isDirty, cacheNode->data );
		free( data );

		//Put in LRU
		if( blockLRUHead == NULL )
		{
			//insert the first node in the double linked list, head.prev = head
			blockLRUHead = cacheNode;
			blockLRUHead->LRUPrev = cacheNode;
			blockLRUHead->LRUNext = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU: Head: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", blockLRUHead->blockNum, (blockLRUHead->LRUPrev)->blockNum, (blockLRUHead->LRUNext)->blockNum, cacheNode->blockNum, (cacheNode->LRUPrev)->blockNum, (cacheNode->LRUNext)->blockNum );
		}
		else
		{
			//insert node at the end of the double linked list
			struct CacheBlockNode *tail = blockLRUHead->LRUPrev;
			tail->LRUNext = cacheNode;
			cacheNode->LRUPrev = tail;
			cacheNode->LRUNext = blockLRUHead;
			blockLRUHead->LRUPrev = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU: Head: %d, prev(%d), next(%d), Tail: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", blockLRUHead->blockNum, (blockLRUHead->LRUPrev)->blockNum, (blockLRUHead->LRUNext)->blockNum, tail->blockNum, (tail->LRUPrev)->blockNum, (tail->LRUNext)->blockNum, cacheNode->blockNum, (cacheNode->LRUPrev)->blockNum, (cacheNode->LRUNext)->blockNum );
		}
		//Put in Hash
		if( blockCacheTable[index] == NULL )
		{
			//insert the first node in the double linked list, head.prev = head
			blockCacheTable[index] = cacheNode;
			blockCacheTable[index]->HashPrev = cacheNode;
			blockCacheTable[index]->HashNext = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: Hash(%d): Head: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", index, blockCacheTable[index]->blockNum, ( blockCacheTable[index] -> HashPrev )->blockNum, (blockCacheTable[index]->HashNext)->blockNum, cacheNode->blockNum, (cacheNode->HashPrev)->blockNum, (cacheNode->HashNext)->blockNum );
		}
		else
		{
			//insert node at the end of the double linked list
			struct CacheBlockNode *tail = blockCacheTable[index]->HashPrev;
			tail->HashNext = cacheNode;
			cacheNode->HashPrev = tail;
			cacheNode->HashNext = blockCacheTable[index];
			blockCacheTable[index]->HashPrev = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: Hash(%d): Head: %d, prev(%d), next(%d), Tail: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", index, blockCacheTable[index]->blockNum, (blockCacheTable[index]->HashPrev)->blockNum, (blockCacheTable[index]->HashNext)->blockNum, tail->blockNum, (tail->HashPrev)->blockNum, (tail->HashNext)->blockNum, cacheNode->blockNum, (cacheNode->HashPrev)->blockNum, (cacheNode->HashNext)->blockNum );
		}
		printHashBlockCache();
	}
	else
	{
		TracePrintf( 100, "[Testing @ yfs.c @ readBlockFromCache]: The block(%d) is in cache, need to move it in LRU\n", blockNum );

		//take it out of the LRU
		struct CacheBlockNode *prev = cacheNode -> LRUPrev;
		struct CacheBlockNode *next = cacheNode -> LRUNext;
		prev -> LRUNext = next;
		next -> LRUPrev = prev;
		if(blockLRUHead == cacheNode)
		{
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: blockLRUHead == cacheNode\n"); 
			if(blockLRUHead == prev && blockLRUHead == next)
			{
				TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: blockLRUHead is the last cacheNode\n"); 
				blockLRUHead = NULL; 
			}
			else
			{
				TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: blockLRUHead is not the last cacheNode\n"); 
				blockLRUHead = next;
			}
		}
		TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU After Remove: Head: %d, prev(%d), next(%d), Current: %d, prev(%d), next(%d), Prev: %d, prev(%d), next(%d), Next: %d, prev(%d), next(%d)\n", blockLRUHead -> blockNum, (blockLRUHead -> LRUPrev) -> blockNum, (blockLRUHead -> LRUNext) -> blockNum, cacheNode->blockNum, (cacheNode->LRUPrev)->blockNum, (cacheNode->LRUNext)->blockNum,prev->blockNum, (prev->LRUPrev)->blockNum, (prev->LRUNext)->blockNum,next->blockNum, (next->LRUPrev)->blockNum, (next->LRUNext)->blockNum );
		printLRUBlockCache();

		//put it at the end of LRU
		if( blockLRUHead == NULL )
		{
			//insert the first node in the double linked list, head.prev = head
			blockLRUHead = cacheNode;
			blockLRUHead->LRUPrev = cacheNode;
			blockLRUHead->LRUNext = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU: Head: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", blockLRUHead->blockNum, (blockLRUHead->LRUPrev)->blockNum, (blockLRUHead->LRUNext)->blockNum, cacheNode->blockNum, (cacheNode->LRUPrev)->blockNum, (cacheNode->LRUNext)->blockNum );
			printLRUBlockCache();
		}
		else
		{
			//insert node at the end of the double linked list
			struct CacheBlockNode *tail = blockLRUHead->LRUPrev;
			tail->LRUNext = cacheNode;
			cacheNode->LRUPrev = tail;
			cacheNode->LRUNext = blockLRUHead;
			blockLRUHead->LRUPrev = cacheNode;
			TracePrintf( 100, "[Testing @ yfs.c @ getBlockFromCache]: LRU: Head: %d, prev(%d), next(%d), Tail: %d, prev(%d), next(%d), New: %d, prev(%d), next(%d)\n", blockLRUHead->blockNum, (blockLRUHead->LRUPrev)->blockNum, (blockLRUHead->LRUNext)->blockNum, tail->blockNum, (tail->LRUPrev)->blockNum, (tail->LRUNext)->blockNum, cacheNode->blockNum, (cacheNode->LRUPrev)->blockNum, (cacheNode->LRUNext)->blockNum );
			printLRUBlockCache();
		}
	}

	return cacheNode;
}

char* readBlockFromCache( int blockNum )
{
	TracePrintf( 100, "[Testing @ yfs.c @ readBlockFromCache]: Start: blockNum(%d)\n", blockNum );

	struct CacheBlockNode *cacheNode = getBlockFromCache( blockNum );
	char *data = malloc( sizeof(char) * BLOCKSIZE );
	memcpy( data, cacheNode->data, BLOCKSIZE );
	TracePrintf( 100, "[Testing @ yfs.c @ readBlockFromCache]: Finish: blockNum(%d), data(%s)\n", blockNum, data );
	return data;
}

int writeBlockToCache( int blockNum, char *data )
{
	TracePrintf( 100, "[Testing @ yfs.c @ writeBlockFromCache]: Start: blockNum(%d), data(%s)\n", blockNum, data );
	struct CacheBlockNode *cacheNode = getBlockFromCache( blockNum );
	memcpy(cacheNode -> data, data, BLOCKSIZE);
	cacheNode -> isDirty = 1;
	return 0;
}

/* Function declarations */
//for general linked list
void insertIntoLinkedIntList( LinkedIntList *node, LinkedIntList **head, LinkedIntList **tail );
LinkedIntList* get( int index, LinkedIntList *head );
void printLinkedList( int level, char *where, LinkedIntList* head );

//for inode and block
int getFreeInode();

int readDirectory( int inodeNum, char *filename, int fileNameLen );
//end of function declarations

/* Implementations */

/* util methods to get freeInode and free blocks */
int getFreeBlock()
{
	LinkedIntList* toPop = isBlockFreeHead;
	isBlockFreeHead = isBlockFreeHead->next;
	//TODO: CHECK NULL
	int num = toPop->num;
	free( toPop ); //from addTo
	return num;
}
int getFreeInode()
{
	LinkedIntList* toPop = isInodeFreeHead;
	isInodeFreeHead = isInodeFreeHead->next;
	if( isInodeFreeHead == NULL )
	{
		TracePrintf( 0, "[Error @ yfs.c @ getFreeInode()]: no free inode left\n" );
	}
	int num = toPop->num;
	free( toPop ); //from addTo
	return num;
}
void addToFreeBlockList( int blockNum )
{
	TracePrintf( 300, "[Testing @ yfs.c @ addToFreeBlockList] adding %d\n", blockNum );
	LinkedIntList* toadd = malloc( sizeof(LinkedIntList) );
	toadd->num = blockNum;
	if( isBlockFreeHead == NULL )
	{
		isBlockFreeHead = toadd;
		isBlockFreeTail = toadd;
		isBlockFreeHead->next = NULL;
	}
	else if( isBlockFreeHead == isBlockFreeTail )
	{
		isBlockFreeTail = toadd;
		isBlockFreeTail->next = NULL;
		isBlockFreeHead->next = isBlockFreeTail;
	}
	else
	{
		isBlockFreeTail->next = toadd;
		toadd->next = NULL;
		isBlockFreeTail = toadd;
	}
}
void addToFreeInodeList( int inodeNum )
{

	TracePrintf( 300, "[Testing @ yfs.c @ addToFreeInodeList] adding %d\n", inodeNum );
	LinkedIntList* toadd = malloc( sizeof(LinkedIntList) );
	toadd->num = inodeNum;
	if( isInodeFreeHead == NULL )
	{
		isInodeFreeHead = toadd;
		isInodeFreeTail = toadd;
		isInodeFreeHead->next = NULL;
	}
	else if( isInodeFreeHead == isInodeFreeTail )
	{
		isInodeFreeTail = toadd;
		isInodeFreeTail->next = NULL;
		isInodeFreeHead->next = isInodeFreeTail;
	}
	else
	{
		isInodeFreeTail->next = toadd;
		toadd->next = NULL;
		isInodeFreeTail = toadd;
	}
}

//begin of linke list methods
void insertIntoLinkedIntList( LinkedIntList* node, LinkedIntList** head, LinkedIntList** tail )
{
	if( *head == NULL )
	{ //change to NULL later
		*head = node;
		node->next = NULL;
		*tail = node;
	}
	else if( *head == *tail )
	{
		*tail = node;
		( *head)->next = *tail;
		node->next = NULL;
	}
	else
	{ //length >= 3
		( *tail)->next = node;
		*tail = node;
	}
}

LinkedIntList* get( int index, LinkedIntList* head )
{
	LinkedIntList* toReturn = head;
	while( toReturn != NULL )
	{
		if( toReturn->index == index )
		{
			return toReturn;
		}
		toReturn = toReturn->next;
	}
	return NULL;
}

void printLinkedList( int level, char *where, LinkedIntList* head )
{
	TracePrintf( level, "[Testing @ %s]: Printing LinkedIntList*:\n", where );
	LinkedIntList* toPrint = head;
	while( toPrint != NULL )
	{
		TracePrintf( level, "[Testing @ %s]: LinkedIntListNode at %p, num %d, next at %p\n", toPrint, toPrint->num, toPrint->next );
		toPrint = toPrint->next;
	}

}
//end of linked list methods

/* Utility functions for printing fs */
void printInode( int level, int inodeNum, char *where, struct inode *inode )
{
	//Print basic information about an inode
	TracePrintf( level, "[Testing @ yfs.c @ %s]: %d, inode(type: %d, nlink: %d, size: %d, direct: %d, indirect: %d)\n", where, inodeNum, inode->type, inode->nlink, inode->size, inode->direct[0], inode->indirect );

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
		free( buf ); //FUCK THIS
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
	TracePrintf( level, "[Testing @ %s @ printDisk] fs_header: num_blocks(%d), num_inodes(%d)\n", where, fsHeader->num_blocks, fsHeader->num_inodes );

	//Print inodesi in blcok 1
	int numOfBlocksContainingInodes = ((fsHeader->num_inodes) + 1) / (BLOCKSIZE / INODESIZE);
	TracePrintf( level, "[Testing @ %s @ printDisk] inodes in %d blocks:\n", where, numOfBlocksContainingInodes );
	int i;
	for( i = 1; i < BLOCKSIZE / INODESIZE; i++ )
	{
		printInode( level, i, "", (struct inode *) buf + i );
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
			for( i = 0; i < BLOCKSIZE / INODESIZE; i++ )
			{
				printInode( level, i, "", (struct inode *) buf + i );
			}
		}
	}
	free( buf );
	TracePrintf( level, "\n" );
}
/* End of utility functions for printing */

//mark used blocks used by inode
int markUsedBlocks( struct inode *inode, int *isBlockFree )
{
	int level = 1100;
	char *where = "yfs.c @ markUserBlocks";

//Print basic information about an inode
	TracePrintf( level, "[Testing @ %s]: inode(type: %d, nlink: %d, size: %d, indirect: %d)\n", where, inode->type, inode->nlink, inode->size, inode->indirect );

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

			free( buf );
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
	numInodes = fsHeader->num_inodes;
	numBlocks = fsHeader->num_blocks;
	int *isInodeFree = malloc( sizeof(int) * numInodes + 1 );
	int *isBlockFree = malloc( sizeof(int) * numBlocks );

	int i;
	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	isInodeFree[0] = USED;
	for( i = 1; i < numInodes + 1; i++ )
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
	for( i = 0; i < numOfBlocksContainingInodes + 1; i++ )
	{
		isBlockFree[i] = USED;
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "BLOCKSIZE/INODESIZE: %d, inodes in %d blocks:\n", BLOCKSIZE / INODESIZE, numOfBlocksContainingInodes );
	int inodeIndex = 1;
	for( i = 1; i < BLOCKSIZE / INODESIZE; i++ )
	{
		int inodeFree = markUsedBlocks( (struct inode *) buf + i, isBlockFree );
		if( inodeFree < 0 )
		{
			TracePrintf( 0, "[Error @ %s ]: Read indirect block %d unsuccessfully\n", where, inodeIndex );
		}

		isInodeFree[inodeIndex] = inodeFree;

//		if( inodeFree == FREE )
//		{
//			//add to the freeList
//			addToFreeInodeList( inodeIndex );
//		}
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
//				if( inodeFree == FREE )
//				{
//				}
				TracePrintf( level, "[Testing @ %s]: inode %d 's status is %d\n", where, inodeIndex, inodeFree );
				inodeIndex++;
			}

			TracePrintf( level, "\n" );
		}
	}

	TracePrintf( level, "[Testing @ %s]: Free Inodes %d:\n", where, numInodes );
	for( i = 0; i < numInodes + 1; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, isInodeFree[i] );
		if( isInodeFree[i] == FREE )
		{
			addToFreeInodeList( i );
		}
	}
	TracePrintf( level, "\n" );

	TracePrintf( level, "[Testing @ %s]: Free Blocks %d:\n", where, numBlocks );
	for( i = 0; i < numBlocks; i++ )
	{
		TracePrintf( level, "%d:%d\n", i, isBlockFree[i] );
		if( isBlockFree[i] == FREE )
		{
			addToFreeBlockList( i );
		}
	}
	TracePrintf( level, "\n" );

//The following code is to check if is linked list works correct
//	while (isInodeFreeHead != NULL)
//	{
//		TracePrintf(600, "[Testing @ yfs.c @ calculateFreeBlocksAndInodes]: free inode: %d\n", isInodeFreeHead -> num);
//		isInodeFreeHead = isInodeFreeHead -> next;
//	}
//
//	while (isBlockFreeHead != NULL)
//	{
//		TracePrintf(600, "[Testing @ yfs.c @ calculateFreeBlocksAndInodes]: free block: %d\n", isBlockFreeHead -> num);
//		isBlockFreeHead = isBlockFreeHead -> next;
//	}
//

	free( buf );
	free( isInodeFree );
	free( isBlockFree );
}

int calculateNumBlocksUsed( int size )
{
	if( size % BLOCKSIZE == 0 )
	{
		TracePrintf( 400, "[Testing @ yfs.c @ calculateNumBlocksUsed]: size (%d), blocks used (%d) without remainder\n", size, size / BLOCKSIZE );
		return size / BLOCKSIZE;
	}
	else
	{
		TracePrintf( 400, "[Testing @ yfs.c @ calculateNumBlocksUsed]: size (%d), blocks used (%d) with remainder\n", size, size / BLOCKSIZE + 1 );
		return size / BLOCKSIZE + 1;
	}
}

/* Given an inodeNumber return a block number */
int getBlockNumInodeIn( int inodeNum )
{
	TracePrintf( 400, "[Testing @ yfs.c @ getWhichBlockInodeIn]: inode (%d) is in block (%d)\n", inodeNum, inodeNum / (BLOCKSIZE / INODESIZE) + 1 );
	return inodeNum / (BLOCKSIZE / INODESIZE) + 1;
}

/* GIven an inodeNumber return a inode index in the block */
int getInodeIndexWithinBlock( int inodeNum )
{
	int inodeIndex = inodeNum % (BLOCKSIZE / INODESIZE);
	TracePrintf( 400, "[Testing @ yfs.c @ getInodeIndexWithinBlock]: inode (%d)'s index is (%d)\n", inodeNum, inodeIndex );
	return inodeIndex;
}

/* Read and write for block */
char* readBlock( int blockNum )
{
	TracePrintf( 100, "[Testing @ yfs.c @ readBlock]: Start: blockNum(%d)\n", blockNum );
	char *buf;
	buf = malloc( sizeof(char) * SECTORSIZE );
	int readBlockStatus = ReadSector( blockNum, buf );
	if( readBlockStatus != 0 )
	{
		TracePrintf( 0, "[Error @ yfs.c @ readBlock]: Read block %d unsuccessfully\n", blockNum );
		return NULL;
	}
	//TODO: free buf at any call to readBlock after use
	return buf;
}

int writeBlock( int blockNum, char *buf )
{
	TracePrintf( 100, "[Testing @ yfs.c @ writeBlock]: Start: blockNum(%d), buf(%s)\n", blockNum, buf );
	int writeBlockStatus = WriteSector( blockNum, buf );
	if( writeBlockStatus != 0 )
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
	if( inodeNum == 0 || inodeNum > numInodes )
	{
		TracePrintf( 0, "[Error @ yfs.c @readInode]: inodeNum %d is illegal\n", inodeNum );
		return NULL;
	}
	//TODO: need to check is the inode Num is valid, if the inode read is correct, should return NULL if encounter any error
	int blockNum = getBlockNumInodeIn( inodeNum );

	char *buf = readBlock( blockNum );
	int inodeIndex = getInodeIndexWithinBlock( inodeNum );
	TracePrintf( 400, "[Testing @ yfs.c @readInode]: blockNum: %d, inodeIndex: %d, sizeof inode: %d\n", blockNum, inodeIndex, sizeof(struct inode) );
	//struct inode* inode = malloc(sizeof(struct inode));
	//TODO: should do some mem copy instead of directly using buf here
	// THIS WILL CAUSE MEMORY LEAK
	// NEED TO FIX LATER
	// malloc inode
	// free(buf)
	// memcpy(inode, buf, blocksize - inodeindex);

	struct inode *inode = malloc( sizeof(struct inode) );

	memcpy( inode, (struct inode *) buf + inodeIndex, sizeof(struct inode) );
//	TracePrintf( 400, "[Testing @ yfs.c @ readInode]: %d: inode(type: %d, nlink: %d, size: %d, direct: %d, indirect: %d)\n", inodeNum, inode->type, inode->nlink, inode->size, inode -> direct[0], inode->indirect );
	printInode( 400, inodeNum, "readInode inode from memcpy", inode );
	free( buf );	//TODO: THIS FREE SEEMS UNSAFE!!
	return inode;
}

struct inode* writeInode( int inodeNum, struct inode *newInode )
{
	int blockNum = getBlockNumInodeIn( inodeNum );

	char *buf = readBlock( blockNum );
	int inodeIndex = getInodeIndexWithinBlock( inodeNum );
	memcpy( (struct inode *) buf + inodeIndex, newInode, sizeof(struct inode) );
	struct inode *inode = newInode;
	TracePrintf( 400, "[Testing @ yfs.c  @ writeInode]: inode after write(type: %d, nlink: %d, size: %d, indirect: %d)\n", inode->type, inode->nlink, inode->size, inode->indirect );
	writeBlock( blockNum, buf );
//TODO: CHECK MEMROY HERE
	free( buf );
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

	int count = 0;
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
			( *usedBlocks)[count] = inode->direct[i];
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
			( *usedBlocks)[count] = inode->direct[i];
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
				( *usedBlocks)[count] = blockIndex;
				count++;
			}
			TracePrintf( level, "\n" );
			free( buf );
		}
	}
	return numBlocks;
}

//return the last directory's inode number
//This method also changes fileName and fileNameCount
int gotoDirectory( int workingDirectoryInodeNumber, char *pathname, int pathNameLen, int* lastExistingDir, int * fileNameCount, char** fileName )
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

	( *lastExistingDir) = lastDirectoryInodeNum;
	TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: start to parse path name\n" );
	//parse the pathname and change the lastDirectoryInodeNum
	for( i = 0; i < MAXPATHNAMELEN; i++ )
	{
//		TracePrintf(500, "[Testing @ yfs.c @ gotoDirectory]: reading c\n");
		c = pathname[i];
		TracePrintf( 500, "[Testing @ yfs.c @ gotoDirectory]: char: %c\n", c );

		if( c == '/' )
		{
			if( ( *fileNameCount) != 0 )
			{
				TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to this directory (%s) len(%d) in the current directory\n", *fileName, *fileNameCount );

				//Change the lastExistingDir inumber
				( *lastExistingDir) = readDirectory( *lastExistingDir, *fileName, *fileNameCount );
				if( *lastExistingDir == ERROR )
				{
					TracePrintf( 300, "[Error @ yfs.c @ gotoDirectory]: path contains : (%s), it is not a directory\n", *fileName );
					return ERROR;
				}
				*fileNameCount = 0;
				memset( *fileName, '\0', DIRNAMELEN );

			}
		}
		else if( c == '\0' )
		{
			TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: parse char 0\n" );
			break;
		}
		else
		{
			TracePrintf( 500, "[Testing @ yfs.c @ gotoDirectory]: writing c\n" );
			( *fileName)[ *fileNameCount] = c;
			( *fileNameCount)++;

			TracePrintf( 500, "[Testing @ yfs.c @ gotoDirectory]: writing c, get %s\n", *fileName );
			if( *fileNameCount >= DIRNAMELEN )
			{
				TracePrintf( 0, "[Error @ yfs.c @ gotoDirectory]: file name length exceeds DIRNAMELEN:(%s)\n", *fileName );
				return ERROR;
			}
		}

	}

	TracePrintf( 300, "[Testing @ yfs.c @ gotoDirectory]: need to go to open or create this file (%s) in the current directory\n", *fileName );

	return lastDirectoryInodeNum;
}

int readDirectory( int inodeNum, char *filename, int fileNameLen )
{
	//need to check if read inode successfully, such as make sure inodeNum is in correct bound
	struct inode *inode = readInode( inodeNum );
	TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: we want to find file(%s), length(%d)\n", filename, fileNameLen );
	TracePrintf( 350, "[Testing @ yfs.c @ readDirectory]: %d: inode(type: %d, nlink: %d, size: %d, direct: %d, indirect: %d)\n", inodeNum, inode->type, inode->nlink, inode->size, inode->direct[0], inode->indirect );
	printInode( 400, inodeNum, "readDirectory", inode );
	int *usedBlocks = malloc( sizeof(int) );		//remember to free this thing somewhere later
	//need to check if get used blocks successfully, the inode may be a free inode
	int usedBlocksCount = getUsedBlocks( inode, &usedBlocks );
	TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: usedBlockCount: %d\n", usedBlocksCount );
	int numDirEntries = (inode->size) / sizeof(struct dir_entry);
	int directoryCount = 0;
	int index = 0;
	int i;
	for( i = 0; i < usedBlocksCount; i++ )
	{
		TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: usedBlock[%d]: %d\n", i, usedBlocks[i] );
		char *buf = readBlock( usedBlocks[i] );
		while( index < BLOCKSIZE / sizeof(struct dir_entry) && directoryCount < numDirEntries )
		{
			struct dir_entry *dirEntry = (struct dir_entry *) buf + index;
			TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: block (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", i, index, directoryCount, dirEntry->inum, dirEntry->name );

			//compare the dir_entry's name and fileName
			int compare = strncmp( dirEntry->name, filename, DIRNAMELEN );
			if( compare == 0 )
			{
				int a = dirEntry->inum;
				//match found
				free( buf );
				return a;
			}

			index++;
			directoryCount++;
		}
		index = 0;
	}
	free( usedBlocks );
	free( inode );
	return ERROR;		//meaning unfound

}

int writeNewEntryToDirectory( int inodeNum, struct dir_entry *newDirEntry )
{
	//need to check if read inode successfully, such as make sure inodeNum is in correct bound
	struct inode *inode = readInode( inodeNum );

	int dirEntryIndex = ((inode->size) % BLOCKSIZE) / sizeof(struct dir_entry);

	if( dirEntryIndex == 0 )
	{
		//need to allocate new data block to store the dir data
		TracePrintf( 350, "[Testing @ yfs.c @ writeNewEntryToDirectory]: need to allocate new data block to store the new dir data\n" );
		//TODO
		int i;
		for( i = 0; i < NUM_DIRECT; i++ )
		{
			if( inode->direct[i] == 0 )
			{
				inode->direct[i] = getFreeBlock();
				if( inode->direct[i] == ERROR )
				{
					TracePrintf( 0, "[Testing @ yfs.c @ writeNewEntryToDirectory]: failed to allocate a new block\n" );
					return ERROR;
				}
				TracePrintf( 0, "[Testing @ yfs.c @ writeNewENtryToDirectory]: allocated new block at %d\n", inode->direct[i] );
				break;
			}
		}
		char *buf = readBlock( inode->direct[i] );
		memcpy( (struct dir_entry*) buf, newDirEntry, sizeof(struct dir_entry) );
		struct dir_entry *dirEntry = (struct dir_entry *) buf;
		TracePrintf( 300, "[Testing @ yfs.c @ writeNewEntryToDirectory]: newblockNum (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", inode->direct[i], dirEntryIndex, dirEntryIndex, dirEntry->inum, dirEntry->name );
		writeBlock( inode->direct[i], buf );

		int originalSize = inode->size;
		int newSize = originalSize + sizeof(struct dir_entry);
		inode->size = newSize;
		TracePrintf( 300, "Testin @ yfs.c @ writeNewEntryToDirectory]: originalSize: %d, newSize: %d\n", originalSize, newSize );
		writeInode( inodeNum, inode );
		free( inode );
		free( buf );
	}
	else
	{
		//can use the same data block to store the dir data
		TracePrintf( 350, "[Testing @ yfs.c @ writeNewEntryToDirectory]: can use the same data block to store the new dir data\n" );
		int *usedBlocks = 0;		//remember to free this thing somewhere later
		//need to check if get used blocks successfully, the inode may be a free inode
		int usedBlocksCount = getUsedBlocks( inode, &usedBlocks );

		int blockNum = usedBlocks[usedBlocksCount - 1];
		TracePrintf( 300, "[Testing @ yfs.c @ writeNewEntryToDirectory]: usedBlockCount: %d, blockNum tobe write: %d\n", usedBlocksCount, blockNum );
		char *buf = readBlock( blockNum );
		memcpy( (struct dir_entry *) buf + dirEntryIndex, newDirEntry, sizeof(struct dir_entry) );
		struct dir_entry *dirEntry = (struct dir_entry *) buf + dirEntryIndex;
		TracePrintf( 300, "[Testing @ yfs.c @ writeNewEntryToDirectory]: block (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", blockNum, dirEntryIndex, dirEntryIndex, dirEntry->inum, dirEntry->name );
		writeBlock( blockNum, buf );
		free( usedBlocks );

		int originalSize = inode->size;
		int newSize = originalSize + sizeof(struct dir_entry);
		inode->size = newSize;
		TracePrintf( 300, "Testin @ yfs.c @ writeNewEntryToDirectory]: originalSize: %d, newSize: %d\n", originalSize, newSize );
		writeInode( inodeNum, inode );
		free( inode );
		free( buf );
	}
	//free
	return 0;
}

/* YFS Server calls corresponding to the iolib calls */
int createFile( int curDir, char *pathname, int pathNameLen )
{
	int lastExistingDir;
	char *fileName = malloc( sizeof(char) * DIRNAMELEN );
	memset( fileName, '\0', DIRNAMELEN );
	int fileNameCount = 0;

	int directoryInodeNum = gotoDirectory( curDir, pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName );
	if( directoryInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ CreateFile]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		free( fileName );
		return ERROR;
	}

	//read directoryInodeNum
	//	int fileInodeNum = readDirectory(workingDirectoryInodeNumber, fileName, fileNameCount);//TODO, not always ROOTINODE

	int fileInodeNum = readDirectory( lastExistingDir, fileName, fileNameCount );	//TODO, not always ROOTINODE
	if( fileInodeNum == ERROR )
	{
		//file not found, make a new file
		//create a newInode for the new file
		int newInodeNum = getFreeInode(); //allocate inode num 
		struct inode *inode = readInode( newInodeNum );
		inode->type = INODE_REGULAR;
		inode->nlink = 1;
		inode->size = 0;
		writeInode( newInodeNum, inode );
		free( inode );
		//create a new dir_entry for the new file
		struct dir_entry *newDirEntry;
		newDirEntry = malloc( sizeof(struct dir_entry) );
		newDirEntry->inum = newInodeNum;
		fileInodeNum = newInodeNum;
		//this part may be substituted by a c libaray function
		memcpy( newDirEntry->name, fileName, DIRNAMELEN );

		TracePrintf( 0, "[Testing @ yfs.c @ CreateFile]: new dir_entry created: inum(%d), name(%s)\n", newDirEntry->inum, newDirEntry->name );
		writeNewEntryToDirectory( lastExistingDir, newDirEntry ); //TODO, not always ROOTINODE
		free( newDirEntry );
	}
	else
	{
		//TODO: Test this part
		//file found, empty the file
		//only tuncate if this file is INODE_REGULAR
		struct inode *inode = readInode( fileInodeNum );
		if( inode->type != INODE_REGULAR )
		{
			TracePrintf( 0, "[Error @ yfs.c @ CreateFile]: file %s exists and is not writable\n", fileName );
			free( fileName );
			return ERROR;
		}
		//TODO: free all the used blocks!!
		//TRUNCATE ALL THE BLOCKS
		//
		free( inode );
	}

	//TODO: clean up fileName and fileNameCount
	fileNameCount = 0;
	free( fileName );
//	fileInodeNum = readDirectory(ROOTINODE, fileName, fileNameCount);//test purpose only, should be delete
	return fileInodeNum;
}

//Open file or a directory

int openFileOrDir( int curDir, char *pathname, int pathNameLen )
{
	int lastExistingDir;
	int fileNameCount = 0;
	char *fileName = malloc( sizeof(char) * DIRNAMELEN );
	//fill in fileName and fileName count;
	memset( fileName, '\0', DIRNAMELEN );
	int directoryInodeNum = gotoDirectory( curDir, pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName );
	if( directoryInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ CreateFile]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		free( fileName );
		return ERROR;

	}

	TracePrintf( 0, "[Testing @ yfs.c @ openFileOrDir] fullpath requested(%s), need(%s), requesting\n", pathname, fileName );
	int fileInodeNum = readDirectory( lastExistingDir, fileName, fileNameCount );

	TracePrintf( 0, "[Testing @ yfs.c @ openFileOrDir] inodeNum(%d), fullpath requested(%s), need(%s), requesting\n", fileInodeNum, pathname, fileName );
	if( fileInodeNum == ERROR )
	{
		TracePrintf( 0, "[Testing @ yfs.c openFileorDir] did not find the file/dir (%s) \n", pathname );
	}
	free( fileName );
	return fileInodeNum;
}
// Make a directory

int mkDir( int curDir, char * pathname, int pathNameLen )
{

	TracePrintf( 0, "[Testing @ yfs.c @ mkDir] Entering Mkdir, requesting %s\n", pathname );
	char *fileName = malloc( sizeof(char) * DIRNAMELEN );
	memset( fileName, '\0', DIRNAMELEN );
	int fileNameCount = 0;

	int lastExistingDir;
	//get to the last eixisting directory
	int directoryInodeNum = gotoDirectory( curDir, pathname, pathNameLen, &lastExistingDir, &fileNameCount, &fileName );

	if( directoryInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ mkDir]: directoryInodeNum is Error: pathname: %s fileName: %s\n", pathname, fileName );
		free( fileName );
		return ERROR;
	}

	TracePrintf( 0, "[Testing @ yfs.c @ mkDir]: try to make directory full path: %s, actual path needs to be created: %s\n", pathname, fileName );
	//fileName is the directory we need to create:

	//int fileInodeNum = readDirectory(lastExistingDir, fileName, fileNameCount);	
	if( fileNameCount == 0 )
	{
		//This means the directory already exists
		TracePrintf( 0, "[Error @ yfs.c @ mkDir]: Trying to create a directory that already exists, pathname(%s)\n", pathname );
		free( fileName );
		return ERROR;
	}
	else
	{
		TracePrintf( 0, "[Testing @ yfs.c @ mkDir]: 946try to make directory full path: %s, actual path needs to be created: %s\n", pathname, fileName );
		//create a new directory with name 

		int fileInodeNum = readDirectory( lastExistingDir, fileName, fileNameCount );
		if( fileInodeNum == ERROR )
		{
			//file does not exist, create a new one
			int newInodeNum = getFreeInode(); //allocate inode num 
			struct inode *inode = readInode( newInodeNum );
			inode->type = INODE_DIRECTORY;
			inode->nlink = 2;
			inode->size = 0;
			writeInode( newInodeNum, inode );
			//create a new dir_entry for the new file
			struct dir_entry *newDirEntry;
			newDirEntry = malloc( sizeof(struct dir_entry) );
			newDirEntry->inum = newInodeNum;
			//copy
			memcpy( newDirEntry->name, fileName, DIRNAMELEN );

			TracePrintf( 0, "[Testing @ yfs.c @ mkDIr]: new dir_entry created: inum(%d), name(%s)\n", newDirEntry->inum, newDirEntry->name );
			/* Add this directory entry to lastExistngDir which is the parent */
			writeNewEntryToDirectory( lastExistingDir, newDirEntry ); //TODO, not always ROOTINODE
			free( newDirEntry );

			/* Add . and .. into the new directory */

			//	writeInode(newInodeNum, inode);
			newDirEntry = malloc( sizeof(struct dir_entry) );
			memset( newDirEntry->name, '\0', DIRNAMELEN );
			memcpy( newDirEntry->name, ".", 1 );
			newDirEntry->inum = newInodeNum;
			TracePrintf( 0, "[Testing @ yfs.c @ mkDir]: added (%s) to the dir we create at %s \n", newDirEntry->name, fileName );
			writeNewEntryToDirectory( newInodeNum, newDirEntry );
			free( newDirEntry );

			//update nlink for lastExistingDir;
			struct inode *lastinode = readInode( lastExistingDir );
			lastinode->nlink += 1;
			writeInode( lastExistingDir, lastinode );
			free( lastinode );
			newDirEntry = malloc( sizeof(struct dir_entry) );
			memset( newDirEntry->name, '\0', DIRNAMELEN );
			memcpy( newDirEntry->name, "..", 2 );
			newDirEntry->inum = lastExistingDir;
			TracePrintf( 0, "[Testing @ yfs.c @ mkDir]: added .. to the dir we create at %s \n", fileName );
			writeNewEntryToDirectory( newInodeNum, newDirEntry );
			free( newDirEntry );
			free( inode );
		}
		else
		{
			TracePrintf( 0, "[Error @ yfs.c @ mkDir]: try to make a direcotry path: %s, but already exists\n", pathname );
			free( fileName );
			return ERROR;
		}
	}

	fileNameCount = 0;
	free( fileName ); //memset(fileName, '\0', DIRNAMELEN);
	return 0;

}

// Change to a new dir
/*
 }*/

int chDir( int curDir, char* pathname, int pathNameLen )
{
	//First open the directory
	int dir = openFileOrDir( curDir, pathname, pathNameLen );
	if( dir == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ chDir] cannot open path %s\n", pathname );
		return ERROR;
	}
	//check if it is a directory node
	struct inode* inode = readInode( dir );
	printInode( 0, dir, "chDir", inode );
	if( inode->type != INODE_DIRECTORY )
	{
		TracePrintf( 0, "[Error @ yfs.c @ chDir] not a directory %s\n", pathname );
		return ERROR;

	}
	free( inode );
	return dir;

}

//remove a directory
/*int rmDir( char* pathname, int pathNameLen )
 }*/

int rmDir( int curDir, char *pathname, int pathnamelen )
{
	//check that directory exists
	int openDir = openFileOrDir( curDir, pathname, pathnamelen );
	if( openDir == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ rmDir]: failed to open directory (%s)\n", pathname );
		return ERROR;
	}

	//TODO:Need Also to remove it in the parent directory
	struct inode * dir = readInode( openDir );
	//If the pathname is a symlink, simply delete the symlink
	if( dir->type == INODE_SYMLINK )
	{
		//TODO: simple kill the symlink
		//unlink(pathname, pathnamelen);
		return 0;
	}
	//check it it is a directory node	
	if( dir->type != INODE_DIRECTORY )
	{
		TracePrintf( 0, "[Error @ yfs.c @ rmDir]: not a directory (%s)\n", pathname );
		return ERROR;
	}

	dir->type = INODE_FREE;		  //set this node free;
	dir->nlink = 0; //TODO: delete all the hard links

	int numBlocks = calculateNumBlocksUsed( dir->size );
	int i;
	for( i = 0; i < NUM_DIRECT; i++ )
	{
		if( dir->direct[i] != 0 )
		{
			addToFreeBlockList( dir->direct[i] );

			TracePrintf( 300, "[Testing @ yfs.c @ rmDir] Freeing direct blocks: (%d)\n", dir->direct[i] );
		}
		dir->direct[i] = 0;
	}

	//handle the indirect blocks
	int numIndirect = numBlocks - NUM_DIRECT;
	if( numIndirect > 0 )
	{
		int blockNum = dir->indirect;
		char *buf;
		buf = malloc( sizeof(char) * SECTORSIZE );
		int readIndirectBlockStatus;
		readIndirectBlockStatus = ReadSector( dir->indirect, buf );
		if( readIndirectBlockStatus != 0 )
		{
			TracePrintf( 0, "[Error @ yfs.c @ rmDir]: Read indirect block %d unsuccessfully\n", dir->indirect );
			return ERROR;
		}
		for( i = 0; i < numIndirect; i++ )
		{
			int blockIndex = *((int *) buf + i);
			TracePrintf( 300, "[Testing @ yfs.c @ rmDir] Freeing indirect blocks: (%d)\n", blockIndex );
			addToFreeBlockList( blockIndex );
		}
		free( buf );
		dir->indirect = 0;
		addToFreeBlockList( blockNum );
	}

	//Now that we have freed all the block used
	//write back the inode free it and return
	dir->size = 0;
	writeInode( openDir, dir );
	free( dir );
	addToFreeInodeList( openDir );

	return 0;
}

int readFile( int inodeNum, int *currentPos, char *buf, int bufSize )
{
	TracePrintf( 200, "[Testing @ yfs.c @ readFile]: Begin: inodeNum: %d, currentPos: %d, bufSize: %d, buf: %s\n", inodeNum, currentPos, bufSize, buf );
	int posInBuf = 0;
	int posInFile = *currentPos;

	struct inode *inode = readInode( inodeNum );
	if( inode == NULL )
	{
		TracePrintf( 0, "[Error @ yfs.c @ readFile]: the inode %d is NULL\n", inodeNum );
		return ERROR;
	}

	if( inode->type == INODE_FREE )
	{
		TracePrintf( 0, "[Error @ yfs.c @ readFile]: the inode %d type is INODE_FREE\n", inodeNum );
		return ERROR;
	}

	int currentFileSize = inode->size;
//	int currentNumBlockUsed = calculateNumBlocksUsed(currentFileSize);

	int *usedBlocks = malloc( sizeof(int) );		//TODO:remember to free this thing somewhere later
	int usedBlocksCount = getUsedBlocks( inode, &usedBlocks );
	TracePrintf( 200, "[Testing @ yfs.c @ readFile]: usedBlockCount: %d\n", usedBlocksCount );

	int blockIndex = calculateNumBlocksUsed( *currentPos );
	if( *currentPos % BLOCKSIZE != 0 )
	{
		//1. read the current block until the block is full
		int numBytesTobeReadInCurrentBlock = BLOCKSIZE - ( *currentPos % BLOCKSIZE);
		if( numBytesTobeReadInCurrentBlock > bufSize )
		{
			numBytesTobeReadInCurrentBlock = bufSize;
		}
		int currentBlockNum = usedBlocks[blockIndex - 1];

		TracePrintf( 200, "[Testing @ yfs.c @ readFile]: Before read the current block: %d, posInBuf: %d\n", currentBlockNum, posInBuf );
		char *currentBlockData = readBlock( currentBlockNum );
		memcpy( buf, currentBlockData + ( *currentPos % BLOCKSIZE), numBytesTobeReadInCurrentBlock );
		TracePrintf( 200, "[Testing @ yfs.c @ readFile]: Current Block %d After reading %d bytes: %s, buf: %s\n", currentBlockNum, numBytesTobeReadInCurrentBlock, currentBlockData, buf );
		free( currentBlockData );

		posInBuf += numBytesTobeReadInCurrentBlock;
		posInFile += numBytesTobeReadInCurrentBlock;
		TracePrintf( 200, "[Testing @ yfs.c @ readFile]: After read the current block: %d, posInBuf: %d, posInFile: %d\n", currentBlockNum, posInBuf, posInFile );
	}

	//2. read the rest of the blocks  
	while( posInBuf < bufSize && posInFile < currentFileSize )
	{
		int numBytesCanBeRead = currentFileSize - posInFile;
		int numBytesTobeRead = bufSize - posInBuf;

		if( numBytesTobeRead > numBytesCanBeRead )
		{
			numBytesTobeRead = numBytesCanBeRead;
		}

		if( numBytesTobeRead > BLOCKSIZE )
		{
			numBytesTobeRead = BLOCKSIZE;
		}

		char *newData = readBlock( usedBlocks[blockIndex] );
		memcpy( buf + posInBuf, newData, numBytesTobeRead );
		TracePrintf( 200, "[Testing @ yfs.c @ readFile]: Current Block %d:%d After reading %d bytes: %s, buf: %s\n", blockIndex, usedBlocks[blockIndex], numBytesTobeRead, newData, buf );
		free( newData );
		posInBuf += numBytesTobeRead;
		posInFile += numBytesTobeRead;
		TracePrintf( 200, "[Testing @ yfs.c @ readFile]: After read the current block: %d:%d, posInBuf: %d, posInFile: %d\n", blockIndex, usedBlocks[blockIndex], posInBuf, posInFile );
		blockIndex++;
	}

	*currentPos = posInFile;
	return posInBuf;
}

int writeFile( int inodeNum, int *currentPos, char *buf, int bufSize )
{
	TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Begin: inodeNum: %d, currentPos: %d, bufSize: %d, buf: %s\n", inodeNum, currentPos, bufSize, buf );

	int posInBuf = 0;
	int posInFile = *currentPos;

	struct inode *inode = readInode( inodeNum );
	if( inode == NULL )
	{
		TracePrintf( 0, "[Error @ yfs.c @ writeFile]: the inode %d is NULL\n", inodeNum );
	}

	if( inode->type == INODE_FREE )
	{
		TracePrintf( 0, "[Error @ yfs.c @ writeFile]: the inode %d type is INODE_FREE\n", inodeNum );
	}

	int currentFileSize = inode->size;
	int newFileSize = *currentPos + bufSize;
	int currentNumBlockNeeded = calculateNumBlocksUsed( currentFileSize );
	int newNumBlockNeeded = calculateNumBlocksUsed( newFileSize );

	if( currentNumBlockNeeded < newNumBlockNeeded )
	{
		//need to expand the file
		int numBlocksNeedToAllocate = newNumBlockNeeded - currentNumBlockNeeded;
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Need to expand the file by %d blocks: currentFileSize: %d, currentNumBlockNeeded: %d, newFileSize: %d, newNumBlockNeeded: %d\n", numBlocksNeedToAllocate, currentFileSize, currentNumBlockNeeded, newFileSize, newNumBlockNeeded );

		if( newNumBlockNeeded <= NUM_DIRECT )
		{
			int i;
			for( i = currentNumBlockNeeded; i < newNumBlockNeeded; i++ )
			{
				inode->direct[i] = getFreeBlock();
			}
		}
		else
		{
			TracePrintf( 0, "[NIY @ yfs.c @ writeFile]: Need to expand to indirect block\n" );
		}
	}
	else if( currentNumBlockNeeded > newNumBlockNeeded )
	{
		//need to truncate the file 
		int numBlocksNeedToFree = currentNumBlockNeeded - newNumBlockNeeded;

		if( currentNumBlockNeeded <= NUM_DIRECT )
		{
			int i;
			for( i = newNumBlockNeeded; i < currentNumBlockNeeded; i++ )
			{
				int tobeFree = inode->direct[i];
				inode->direct[i] = 0;
				addToFreeBlockList( tobeFree );		//TODO: clean the block content before free the block
			}
		}
		else
		{
			TracePrintf( 0, "[NIY @ yfs.c @ writeFile]: Need to expand to indirect block\n" );
		}
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Need to truncate the file by %d blocks: currentFileSize: %d, currentNumBlockNeeded: %d, newFileSize: %d, newNumBlockNeeded: %d\n", numBlocksNeedToFree, currentFileSize, currentNumBlockNeeded, newFileSize, newNumBlockNeeded );
	}
	else
	{
		//need not adjust the blocks
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Need not adjust the file: currentFileSize: %d, currentNumBlockNeeded: %d, newFileSize: %d, newNumBlockNeeded: %d\n", currentFileSize, currentNumBlockNeeded, newFileSize, newNumBlockNeeded );
	}

	inode->size = newFileSize;
	writeInode( inodeNum, inode );
	printInode( 200, inodeNum, "writeFile", inode );
	free( inode );

	int *usedBlocks = malloc( sizeof(int) );		//TODO:remember to free this thing somewhere later
	int usedBlocksCount = getUsedBlocks( inode, &usedBlocks );
	TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: usedBlockCount: %d\n", usedBlocksCount );

	int blockIndex = calculateNumBlocksUsed( *currentPos );
	if( *currentPos % BLOCKSIZE != 0 )
	{
		//1. write the current block until the block is full
		int numBytesTobeWriteInCurrentBlock = BLOCKSIZE - ( *currentPos % BLOCKSIZE);
		if( numBytesTobeWriteInCurrentBlock > bufSize )
		{
			numBytesTobeWriteInCurrentBlock = bufSize;
		}
		int currentBlockNum = usedBlocks[blockIndex - 1];

		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Before write the current block: %d, posInBuf: %d\n", currentBlockNum, posInBuf );
		char *currentBlockData = readBlock( currentBlockNum );
		memcpy( currentBlockData + ( *currentPos % BLOCKSIZE), buf, numBytesTobeWriteInCurrentBlock );
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Current Block %d After Writing %d bytes: %s\n", currentBlockNum, numBytesTobeWriteInCurrentBlock, currentBlockData );
		writeBlock( currentBlockNum, currentBlockData );
		free( currentBlockData );

		posInBuf += numBytesTobeWriteInCurrentBlock;
		posInFile += numBytesTobeWriteInCurrentBlock;
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: After write the current block: %d, posInBuf: %d, posInFile:%d\n", currentBlockNum, posInBuf, posInFile );
	}

	//2. write the rest of the blocks  
	while( posInBuf < bufSize )
	{
		int numBytesTobeWrite = bufSize - posInBuf;
		if( numBytesTobeWrite > BLOCKSIZE )
		{
			numBytesTobeWrite = BLOCKSIZE;
		}

		char *newData = malloc( sizeof(char) * BLOCKSIZE );
		memcpy( newData, buf + posInBuf, numBytesTobeWrite );
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: Current Block %d:%d After Writing %d bytes: %s\n", blockIndex, usedBlocks[blockIndex], numBytesTobeWrite, newData );
		writeBlock( usedBlocks[blockIndex], newData );
		free( newData );
		posInBuf += numBytesTobeWrite;
		posInFile += numBytesTobeWrite;
		TracePrintf( 200, "[Testing @ yfs.c @ writeFile]: After write the current block: %d:%d, posInBuf: %d, posInFile:%d\n", blockIndex, usedBlocks[blockIndex], posInBuf, posInFile );
		blockIndex++;
	}

	*currentPos = posInFile;
	return posInBuf;
}

/* Link implementation at server side
 */
int link( int curDir, char *oldname, int oldnamelen, char* newname, int newnamelen )
{
	//TracePrintf( 0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n" );	
	int lastExistingDir;
	char *fileName = malloc( sizeof(char) * DIRNAMELEN );
	int fileNameCount = 0;
	TracePrintf( 0, "[Testing @ yfs.c @ link]:Entering link, reqeusting oldname(%s), newname(%s)\n", oldname, newname );

	int oldInodeNum = openFileOrDir( curDir, oldname, oldnamelen );
	if( oldInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ link]: Cannot find oldpathname:(%s)\n", oldname );
		free( fileName );
		//symLink(oldname, oldnamelen, newname, newnamelen);
		return ERROR;
	}

	struct inode* oldnode = readInode( oldInodeNum );
	if( oldnode->type == INODE_DIRECTORY )
	{
		TracePrintf( 0, "[Error @ yfs.c @ link]: oldpathname is a directory(%s)\n", oldname );
		free( fileName );
		return ERROR;
	}
	/* Need to check if new path already exists */

	int checkNewPath = openFileOrDir( curDir, newname, newnamelen );
	if( checkNewPath != ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ link]: newpathname already exists(%s) inodenum(%d)\n", newname, checkNewPath );
		free( fileName );
		return ERROR;
	}

	//TODO: pass in the current directory
	int findDir = gotoDirectory( curDir, newname, newnamelen, &lastExistingDir, &fileNameCount, &fileName );
	if( findDir == ERROR )
	{
		TracePrintf( 0, "[Testing @ yfs.c @ link]: did not find last dir in newname %s\n", fileName );
		free( fileName );
		return ERROR;
	}
	if( fileNameCount != 0 )
	{
		TracePrintf( 0, "[Testing @ yfs.c @ link]: write a new file as (%s), fullpath for newnae(%s)\n", fileName, newname );
		struct inode* lastDir = readInode( lastExistingDir );

		//create a new dir_entry for the new file
		struct dir_entry *newDirEntry;
		newDirEntry = malloc( sizeof(struct dir_entry) );
		newDirEntry->inum = oldInodeNum;
		//this part may be substituted by a c libaray function
		memcpy( newDirEntry->name, fileName, DIRNAMELEN );

		TracePrintf( 0, "[Testing @ yfs.c @ link]: new dir_entry created: inum(%d), name(%s)\n", newDirEntry->inum, newDirEntry->name );
		writeNewEntryToDirectory( lastExistingDir, newDirEntry ); //TODO, not always ROOTINODE
		free( newDirEntry );
		free( lastDir );
	}
	else
	{

		//TODO: should never reach here
		TracePrintf( 0, "[Testing @ yfs.c @ Link]: \n" );
		free( oldnode );
		free( fileName );
		return ERROR;
	}

	//add to nlink in old node
	oldnode->nlink++;
	writeInode( oldInodeNum, oldnode );
	free( oldnode );
	free( fileName );
	return 0;

}

int unlink( int curDir, char * pathname, int pathnamelen, struct Message * msg )
{
	//TracePrintf( 0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n" );
	TracePrintf( 0, "[Testing @ yfs.c @ unlink]: Entering unlik for (%s) pathname\n", pathname );

	char *fileName = malloc( sizeof(char) * DIRNAMELEN );
	int fileNameCount = 0;
	int lastExistingDir;

	int findDir = gotoDirectory( curDir, pathname, pathnamelen, &lastExistingDir, &fileNameCount, &fileName );
	if( findDir == ERROR )
	{
		//need to contii
		//nue to search for file in the current dir	
		TracePrintf( 0, "[Error @ yfs.c @ unlink]: bad gotoDictory\n" );
		free( fileName );
		return ERROR;
	}

	if( fileNameCount == 0 )
	{
		TracePrintf( 0, "[Error @ yfs.c @ unlink]: the pathname ends with a directory %s pathnam\n" );
		free( fileName );
//}
		return ERROR;
	}
	else
	{
		struct inode *inode = readInode( lastExistingDir );
		TracePrintf( 300, "[Testing @ yfs.c @ unlink]: we want to find file(%s), length(%d)\n", fileName, fileNameCount );
		TracePrintf( 350, "[Testing @ yfs.c @ unlink]: %d: inode(type: %d, nlink: %d, size: %d, direct: %d, indirect: %d)\n", lastExistingDir, inode->type, inode->nlink, inode->size, inode->direct[0], inode->indirect );
		int *usedBlocks = malloc( sizeof(int) );		//remember to free this thing somewhere later

		//need to check if get used blocks successfully, the inode may be a free inode
		int usedBlocksCount = getUsedBlocks( inode, &usedBlocks );
		TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: usedBlockCount: %d\n", usedBlocksCount );
		int numDirEntries = (inode->size) / sizeof(struct dir_entry);
		int directoryCount = 0;
		int index = 0;
		int i;
		for( i = 0; i < usedBlocksCount; i++ )
		{
			TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: usedBlock[%d]: %d\n", i, usedBlocks[i] );
			char *buf = readBlock( usedBlocks[i] );
			while( index < BLOCKSIZE / sizeof(struct dir_entry) && directoryCount < numDirEntries )
			{
				struct dir_entry *dirEntry = (struct dir_entry *) buf + index;
				TracePrintf( 300, "[Testing @ yfs.c @ readDirectory]: block (%d), index(%d), directory[%d]: inum(%d), name(%s)\n", i, index, directoryCount, dirEntry->inum, dirEntry->name );

				//compare the dir_entry's name and fileName
				int compare = strncmp( dirEntry->name, fileName, DIRNAMELEN );
				if( compare == 0 )
				{
					//found the entry
					struct inode *oldnode = readInode( dirEntry->inum );
					oldnode->nlink--;
					if( oldnode->nlink == 0 )
					{
						//free this node

					}
					writeInode( dirEntry->inum, oldnode );
					free( oldnode );
					dirEntry->inum = 0;

					writeBlock( usedBlocks[i], buf );
					free( buf );
					free( inode );
					free( fileName );
					free( usedBlocks );
					return 0;

				}

				index++;
				directoryCount++;
			}

			free( buf );

		}

		free( usedBlocks );
		free( inode );
	}
	free( fileName );
	return ERROR;

}

int symLink( int curDir, char *oldname, int oldnamelen, char* newname, int newnamelen )
{
//	TracePrintf( 0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n" );
	TracePrintf( 0, "[Testing @ yfs.c @ symLink]: Entering symLink: oldname(%s) oldlen(%d), newname(%s), newlen(%d)\n", oldname, oldnamelen, newname, newnamelen );

	//first create the symlink file
	int newSymLinkInodeNum = createFile( curDir, newname, newnamelen );

	if( newSymLinkInodeNum == ERROR )
	{
		TracePrintf( 0, "[Error @ yfs.c @ symLink]: Unable to create newpathname: (%s)\n", newname );
		return ERROR;
	}

	struct inode * symLinkNode = readInode( newSymLinkInodeNum );
	symLinkNode->type = INODE_SYMLINK;
//TODO: WRITE THE OLDNAME INTO THE Symlinknode;
	int curPos = 0;
	writeFile( newSymLinkInodeNum, &curPos, oldname, oldnamelen );
	writeInode( newSymLinkInodeNum, symLinkNode );
	free( symLinkNode );
	return 0;

}

int readLink( struct Message *msg )
{
	TracePrintf( 0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n" );
	return 0;

}

int seek( int inodeNum )
{
	struct inode *inode = readInode( inodeNum );
	if( inode == NULL )
	{
		TracePrintf( 0, "[Error @ yfs.c @ seek]: the inode: %d is NULL\n", inodeNum );
		return ERROR;
	}
	int size = inode->size;
	free( inode );
	return size;
}

struct Stat *getFileStat( int curDir, char *pathname, int pathNameLen )
{
	int inodeNum = openFileOrDir( curDir, pathname, pathNameLen );

	struct inode *inode = readInode( inodeNum );
	if( inode == NULL )
	{
		TracePrintf( 0, "[Error @ yfs.c @ stat]: the inode %d is NULL\n", inodeNum );
	}

	if( inode->type == INODE_FREE )
	{
		TracePrintf( 0, "[Error @ yfs.c @ stat]: the inode %d type is INODE_FREE\n", inodeNum );
	}

	struct Stat *stat = malloc( sizeof(struct Stat) );
	stat->inum = inodeNum;
	stat->type = inode->type;
	stat->size = inode->size;
	stat->nlink = inode->nlink;

	TracePrintf( 150, "[Testing @ yfs.c @ Stat]: Finished: pathname: %s, pathNameLen: %d, Stat: inum(%d), type(%d), size(%d), nlink(%d)\n", pathname, pathNameLen, stat->inum, stat->type, stat->size, stat->nlink );

	return stat;
}

int sync()
{

	TracePrintf( 0, "[THIS FUNCTION IS NOT IMPLEMENTED]\n" );
	return 0;
//SYNC THE CACHE
}
/* End of yfs server calls */

void addressMessage( int pid, struct Message *msg )
{
	int type = msg->messageType;
	TracePrintf( 500, "[Testing @ yfs.c @ receiveMessage]: receive message typed %d\n", type );

	int len;
	char *pathname = 0;
	int size;
	int pathnameMalloc = 0;
	int bufMalloc = 0;
	char *buf = 0;
	int inode;
	int currentPos;
	int curDir = ROOTINODE;
	if( type == OPEN || type == CREATE || type == UNLINK || type == MKDIR || type == RMDIR || type == CHDIR || type == STAT || type == READLINK || type == LINK || type == SYMLINK )
	{
		//get pathname and len
		len = msg->len;
		pathname = malloc( sizeof(char) * len );
		pathnameMalloc = 1;
		//TracePrintf(0, "[Testing @ yfs.c @ addressMessage]: pathname ptr (%p), pathname(%s)\n", msg->pathname, msg->pathname);
		int copyFrom = CopyFrom( pid, pathname, msg->pathname, len );

		TracePrintf( 0, "[Testing @ yfs.c @ addressMessage]: pathname ptr (%p), pathname(%s)\n", pathname, pathname );
		if( copyFrom == ERROR )
		{
			TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy pathname from pid %d failure\n", pid );
		}
	}

	if( type == READ || type == WRITE || type == LINK || type == SYMLINK || type == READLINK )
	{
		//get buf and size
		size = msg->size;
		buf = malloc( sizeof(char) * size );
		bufMalloc = 1;
		int copyFrom = CopyFrom( pid, buf, msg->buf, size );
		if( copyFrom == ERROR )
		{
			TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy buf from pid %d failure, size: %d, buf: %d, msg->buf: %d\n", pid, size, buf, msg->buf );
		}
	}

	if( type == READ || type == WRITE || type == SEEK )
	{
		//get inode and pos
		inode = msg->inode;
		currentPos = msg->len;
	}

	curDir = msg->inode;		//NOTEl DO NOT USE THIS IN READ WIRTE SEEK
	switch( type )
	{
		case OPEN:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message OPEN: type(%d), len(%d), pathname(%s)\n", type, len, pathname );

			//TODO:OPEN
			msg->inode = openFileOrDir( curDir, pathname, len );
			break;
		case CREATE:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CREATE: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			msg->inode = createFile( curDir, pathname, len );
			break;
		case UNLINK:
			//TODO: UNLINK
			//
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message UNLINK: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			unlink( curDir, pathname, len, msg );
			break;
		case MKDIR:
			//TODO: MIDIR
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message MKDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			msg->size = mkDir( curDir, pathname, len );
			break;
		case RMDIR:
			//TODO: RMDIRi
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message RMDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			msg->size = rmDir( curDir, pathname, len );
			break;
		case CHDIR:
			//TODO: CHDIR

			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message CHDIR: type(%d), len(%d), pathname(%s)\n", type, len, pathname );
			msg->size = chDir( curDir, pathname, len );
			break;
		case STAT:
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message STAT: type(%d), len(%d), pathname(%s), statbuf(%d)\n", type, len, pathname, msg->buf );
			struct Stat *stat = getFileStat( curDir, pathname, len );
			if( stat == NULL )
			{
				msg->size = ERROR;
			}

			if( CopyTo( pid, msg->buf, stat, sizeof(struct Stat) ) == ERROR )
			{
				TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy stat to pid %d failure, size: %d, stat: %d, msg->buf: %d\n", pid, size, stat, msg->buf );
			}
			free( stat );
			break;
		case READLINK:
			//TODO: READLINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message READLINK: type(%d), len(%d), pathname(%s), size(%d), buf(%s)\n", type, len, pathname, size, buf );
			break;
		case LINK:
			//TODO: LINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message LINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n", type, len, pathname, size, buf );
			msg->size = link( curDir, pathname, len, buf, size );
			break;
		case SYMLINK:
			//TODOZ: SYMLINK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYMLINK: type(%d), oldLen(%d), oldName(%s), newLen(%d), newName(%s)\n", type, len, pathname, size, buf );

			symLink( curDir, pathname, len, buf, size );
			break;
		case READ:
			//TODO: READ
			TracePrintf( 200, "[Testing @ yfs.c @ addressMessage]: Message READ: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode, currentPos, size, buf );
			msg->size = readFile( inode, &currentPos, buf, size );
			msg->len = currentPos;
			if( CopyTo( pid, msg->buf, buf, size ) == ERROR )
			{
				TracePrintf( 0, "[Error @ yfs.c @ addressMessage]: copy buf to pid %d failure, size: %d, buf: %d, msg->buf: %d\n", pid, size, buf, msg->buf );
			}
			TracePrintf( 200, "[Testing @ yfs.c @ addressMessage]: Reply READ: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode, currentPos, size, buf );
			break;
		case WRITE:
			//TODO: WRITE
			TracePrintf( 200, "[Testing @ yfs.c @ addressMessage]: Message WRITE: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode, currentPos, size, buf );
			msg->size = writeFile( inode, &currentPos, buf, size );
			msg->len = currentPos;
			TracePrintf( 200, "[Testing @ yfs.c @ addressMessage]: Reply WRITE: type(%d), inode(%d), pos(%d), size(%d), buf(%s)\n", type, inode, currentPos, size, buf );
			break;
		case SEEK:
			//TODO: SEEK
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SEEK: type(%d), inode(%d)\n", type, inode );
			msg->size = seek( inode );
			break;
		case SYNC:
			//TODO: SYNC
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SYNC: type(%d)\n", type );
			break;
		case SHUTDOWN:
			//TODO: SHUTDOWN
			TracePrintf( 500, "[Testing @ yfs.c @ addressMessage]: Message SHUTDOWN: type(%d)\n", type );
			TtyWrite( 0, "YFS shutting down now", 22 );
			printf( "YFS shutting down now" );
			Exit( 0 );
			break;
	}
	if( pathnameMalloc == 1 )
	{
		free( pathname );
		pathnameMalloc = 0;
	}
	if( bufMalloc == 1 )
	{
		free( buf );
		bufMalloc = 0;
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

	printDisk( 1500, "main.c" );
	calculateFreeBlocksAndInodes();

	int pid = Fork();
	if( pid == 0 )
	{
		Exec( argv[1], argv + 1 );
	}
	else
	{
		while( 1 )
		{
			struct Message *msg = malloc( sizeof(struct Message) );
			TracePrintf( 500, "loop\n" );
			int sender = Receive( msg );
			if( sender == ERROR )
			{
				TracePrintf( 0, "[Error @ yfs.c @ main]: Receive Message Failure\n" );
				return ERROR;
			}

			TracePrintf( 500, "Sender: %d\n", sender );

			char *buf = readBlockFromCache( 1 );
			buf = readBlockFromCache( 2 );
			buf = readBlockFromCache( 3 );
			buf = readBlockFromCache( 4 );
			buf = readBlockFromCache( 33 );
			buf = readBlockFromCache( 65 );
			buf = readBlockFromCache( 97 );
			writeBlockToCache( 1, buf );
			writeBlockToCache( 3, buf );
			free( buf );
//			addressMessage( sender, msg );
//			Reply( msg, sender );
			free( msg );
		}
	}

	return 0;
}
