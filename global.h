#ifndef _global_h
#define _global_h

//When scan the free lists of inodes and blocks
#define FREE 0
#define USED 1

//message type
#define OPEN 1
#define CLOSE 2
#define CREATE 3
#define READ 4
#define WRITE 5
#define SEEK 6
#define LINK 7
#define UNLINK 8
#define SYMLINK 9
#define READLINK 10
#define MKDIR 11
#define RMDIR 12
#define CHDIR 13
#define STAT 14
#define SYNC 15
#define SHUTDOWN 16

struct Message
{
	int messageType;
	int fd; //inum
	int len; //offset or nlink
	int size; //whence
	char *pathname; //oldname
	char *buf; //newname
};

#endif
