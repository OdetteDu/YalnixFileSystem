#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdint.h>
#include "global.h"

extern int Open(char *pathname)
{
	struct Message msg;
	msg.messageType = OPEN;
	msg.pathname = pathname;
	msg.len = strlen(pathname); 

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}

	return 0;	
}

extern int Close(int fd)
{
	struct Message msg;
	msg.messageType = CLOSE;
	msg.fd = fd;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;	
}

extern int Read(int fd, void *buf, int size)
{
	struct Message msg;
	msg.messageType = READ;
	msg.fd = fd;
	msg.buf = buf;
	msg.size = size;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;	
}

extern int Write(int fd, void *buf, int size)
{
	struct Message msg;
	msg.messageType = WRITE;
	msg.fd = fd;
	msg.buf = buf;
	msg.size = size;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	  
	return 0;	
}

extern int Seek(int fd, int offset, int whence)
{
	struct Message msg;
	msg.messageType = SEEK;
	msg.fd = fd;
	msg.len = offset;
	msg.size = whence;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
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
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;
}

extern int Stat(char *pathname, struct Stat *statbuf)
{
	struct Message msg;
	msg.messageType = STAT;
	msg.pathname = pathname;
	msg.len = strlen(pathname);

	msg.size = statbuf -> size;
	msg.fd = statbuf -> type;
	int *data = malloc(sizeof(int) * 2);
	data[0] = statbuf -> inum;
	data[1] = statbuf -> nlink;
	msg.buf = (*data);

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;	
}

extern int Sync(void)
{
	struct Message msg;
	msg.messageType = SYNC;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;	
}

extern int Shutdown(void)
{
	struct Message msg;
	msg.messageType = SHUTDOWN;

	int send = Send((void *)&msg, FILE_SERVER);
	if( send != 0 )
	{
		TracePrintf(0, "[Error @ iolib.h @ Open]: The send status is Error.\n");
	}
	return 0;	
}

