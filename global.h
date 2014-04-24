#ifndef _global_h
#define _global_h

//When scan the free lists of inodes and blocks
#define FREE 0
#define USED 1

//message type
#define OPEN 0 
#define CLOSE 1
#define CREATE 2 
#define READ 3
#define WRITE 4
#define SEEK 5
#define LINK 6
#define UNLINK 7
#define SYMLINK 8
#define READLINK 9
#define MKDIR 10
#define RMDIR 11
#define CHDIR 12
#define STAT 13
#define SYNC 14
#define SHUTDOWN 15

struct Message
{
	int messageType;
	int returnStatus;
	int inode;
	int len; //currentPos
	int size;
	char *pathname; //oldname
	char *buf; //newname
};

typedef struct linkedIntList{
	int num;
	struct linkedIntList *next;
	int index; 
}LinkedIntList;

LinkedIntList* get(int index, LinkedIntList* listHead);
void insertIntoLinkedIntList(LinkedIntList* node ,  LinkedIntList** listHead ,  LinkedIntList** listTail);

#endif
