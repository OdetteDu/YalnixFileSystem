#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>

#include "global.h"

struct OpenFileEntry
{
	int status;
	int inode;
	int currentPos;
};

struct OpenFileEntry openFileTable[MAX_OPEN_FILES];

int isFileDescriptorLegal(int fd)
{
	if(fd < 0 || fd >= MAX_OPEN_FILES)
	{
		TracePrintf(0, "[Error @ iolib.c @ isFileDescriptorLegal]: The fd %d is < 0 or > %d\n", fd, MAX_OPEN_FILES);
		return ERROR;
	}

	if(openFileTable[fd].status == FREE)
	{
		TracePrintf(0, "[Error @ iolib.c @ isFileDescriptorLegal]: The fd %d is free\n");
		return ERROR;
	}

	return 0;
}

extern int Open(char *pathname)
{
	struct Message msg;
	msg.messageType = OPEN;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	TracePrintf(500, "before blocked from send\n");
	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	
	TracePrintf(500, "Unblocked from send\n");

	return 0;	
}

extern int Close(int fd)
{
	return 0;	
}

extern int Create(char *pathname)
{
	struct Message msg;
	msg.messageType = CREATE;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Create]: The send status is Error.\n");
	}
	return 0;	
}

extern int Read(int fd, void *buf, int size)
{
	if(isFileDescriptorLegal(fd) != 0)
	{
		TracePrintf(0, "[Error @ iolib.c @ Read]: The fd %d is not legal\n", fd);
		return ERROR;
	}
	struct Message msg;
	msg.messageType = READ;
	msg.inode = openFileTable[fd].inode;
	msg.len = openFileTable[fd].currentPos;
	msg.buf = buf;
	msg.size = size;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Read]: The send status is Error.\n");
	}
	return 0;	
}

extern int Write(int fd, void *buf, int size)
{
	if(isFileDescriptorLegal(fd) != 0)
	{
		TracePrintf(0, "[Error @ iolib.c @ Write]: The fd %d is not legal\n", fd);
		return ERROR;
	}
	struct Message msg;
	msg.messageType = WRITE;
	msg.inode = openFileTable[fd].inode;
	msg.len = openFileTable[fd].currentPos;
	msg.buf = buf;
	msg.size = size;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Write]: The send status is Error.\n");
	}
	return 0;	
}

extern int Seek(int fd, int offset, int whence)
{
	return 0;	
}

extern int Link(char *oldname, char *newname)
{
	struct Message msg;
	msg.messageType = LINK;
	msg.pathname = oldname;
	msg.len = strlen(oldname); 
	msg.buf = newname;
	msg.size = strlen(newname);

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Link]: The send status is Error.\n");
	}
	return 0;	
}

extern int Unlink(char *pathname)
{
	struct Message msg;
	msg.messageType = UNLINK;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Unlink]: The send status is Error.\n");
	}
	return 0;	
}

extern int SymLink(char *oldname, char *newname)
{
	struct Message msg;
	msg.messageType = SYMLINK;
	msg.pathname = oldname;
	msg.len = strlen(oldname); 
	msg.buf = newname;
	msg.size = strlen(newname);

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ SymLink]: The send status is Error.\n");
	}
	  
	return 0;	
}

extern int ReadLink(char *pathname, char *buf, int size)
{
	struct Message msg;
	msg.messageType = READLINK;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 
	msg.buf = buf;
	msg.size = strlen(buf);

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ ReadLink]: The send status is Error.\n");
	}
	return 0;	
}

extern int MkDir(char *pathname)
{
	struct Message msg;
	msg.messageType = MKDIR;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ MkDir]: The send status is Error.\n");
	}
	return 0;	
}

extern int RmDir(char *pathname)
{
	struct Message msg;
	msg.messageType = RMDIR;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ RmDir]: The send status is Error.\n");
	}
	return 0;
}

extern int Stat(char *pathname, struct Stat *statbuf)
{
	struct Message msg;
	msg.messageType = STAT;
	msg.pathname = pathname;
	msg.len = strlen(pathname);
	msg.size = sizeof(statbuf); //unfinished

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Stat]: The send status is Error.\n");
	}
	return 0;	
}

extern int Sync()
{
	struct Message msg;
	msg.messageType = SYNC;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Sync]: The send status is Error.\n");
	}
	return 0;	
}

extern int Shutdown()
{
	struct Message msg;
	msg.messageType = SHUTDOWN;

	int send = Send(&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Shutdown]: The send status is Error.\n");
	}

	TracePrintf(500, "[Testing @ iolib.h @ Shutdown]: Reply: messageType(%d)\n", msg.messageType);
	return 0;	
}

