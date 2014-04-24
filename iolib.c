#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <stdint.h>
#include "global.h"

struct OpenFileEntry
{
	int fd;
	int isOpen;
	int inode;
	int currentPos;
//	int status;
};

struct OpenFileEntry openFileTable[MAX_OPEN_FILES];

/* Function declarations */
int isFileDescriptorLegal( int fd );
int getFreeFD( void );
//int checkFileOpen( int fd );
int getPathnameLength( char *buf );

/* Start of implementation */
int getFreeFD()
{
	int i;
	for( i = 0; i < MAX_OPEN_FILES; i++ )
	{
		if( openFileTable[i].isOpen == 0 )
		{
			//openFileTable[i].isOpen = 1;
			return i;
		}
	}
	TracePrintf( 0, "[Error @ iolib.c @ getFreeFD]: Reached MAX_OPEN_FILES\n" );
	return ERROR; //did not find a free fd
}

//int checkFileOpen( int fd )
//{
//	//presumably the fd is the index of the file entry in our static array
//	if( openFileTable[fd].isOpen == 1 )
//		return fd;
//
//	TracePrintf( 0, "[Error @ iolib.c @ checkFileOpen]: Given fd(%d) isOpen parameter unset\n", fd );
//	return ERROR;
//}

/* checks if the path name given exceeds the maximum path name length */
int getPathnameLength( char *buf )
{
	int length = 0;
	char *name = buf;
	while( ( *name) != '\0' )
	{
		length++;
		if( length >= MAXPATHNAMELEN )
		{

			TracePrintf( 0, "[Error @ iolib.c @ getPathnameLength]: Given path name is longer than 32\n" );
			return ERROR;
		}
		name++;
	}
	return length;
}

int isFileDescriptorLegal( int fd )
{
	if( fd < 0 || fd >= MAX_OPEN_FILES )
	{
		TracePrintf( 0, "[Error @ iolib.c @ isFileDescriptorLegal]: The fd %d is < 0 or > %d\n", fd, MAX_OPEN_FILES );
		return ERROR;
	}

	if( openFileTable[fd].isOpen == 0 )
	{
		TracePrintf( 0, "[Error @ iolib.c @ isFileDescriptorLegal]: The fd %d is free\n" );
		return ERROR;
	}

	return 0;
}

extern int Open( char *pathname )
{
	struct Message msg;
	int newfd, length;
	char *pathCopy;

	TracePrintf( 0, "[Testing @ iolib.c @ Open]: Entering open\n" );
	if( (newfd = getFreeFD()) == ERROR )
		return ERROR; //get new fd
	if( (length = getPathnameLength( pathname )) == ERROR )
		return ERROR; //get path length, no \0
	//make a new copy of the path name;
	if( (pathCopy = malloc( (length + 1) * sizeof(char) )) == NULL )
	{
		TracePrintf( 0, "[Error @ iolib.c @ Open]: Failed to malloc for pathname copy\n" );
		return ERROR;
	}
	memcpy( pathCopy, pathname, length + 1 );

	TracePrintf( 0, "[Testing @ iolib.c @ Open]: copied path name\n" );
	openFileTable[newfd].fd = newfd;
	openFileTable[newfd].isOpen = 1;

	msg.messageType = OPEN;
	msg.pathname = pathCopy;
	msg.len = length + 1;	//strlen(pathname);

	TracePrintf( 500, "before blocked from send\n" );
	int send = Send( &msg, FILE_SERVER );
	if( send == ERROR )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Open]: The send status is Error.\n" );
		return ERROR;
	}

	openFileTable[newfd].inode = msg.inode;	//record the inode number of the open file
	TracePrintf( 0, "[Testing @ iolib.c @ Open]: opened the file with inode number: %d\n", msg.inode );
	TracePrintf( 0, "[Testing @ iolib.c @ Open] : Unblocked from send\n" );
	free( pathCopy );
	return newfd;
}

extern int Close( int fd )
{
	if( isFileDescriptorLegal( fd ) == ERROR )
		return ERROR;
	openFileTable[fd].isOpen = 0;
	openFileTable[fd].inode = -1;
	TracePrintf( 0, "[Testing @ iolib.c @ Close]: Closed file fd(%d)\n", fd );
	return 0;
}

extern int Create( char *pathname )
{
	struct Message msg;
	int newfd, length;
	char *pathCopy;

	TracePrintf( 0, "[Testing @ iolib.c @ Create]: Entering create\n" );
	if( (newfd = getFreeFD()) == ERROR )
		return ERROR;	//get new fd
	if( (length = getPathnameLength( pathname )) == ERROR )
		return ERROR;	//get path length, no \0

	//make a new copy of the path name;
	if( (pathCopy = malloc( (length + 1) * sizeof(char) )) == NULL )
	{
		TracePrintf( 0, "[Error @ iolib.c @ Create]: Failed to malloc for pathname copy\n" );
		return ERROR;
	}
	memcpy( pathCopy, pathname, length + 1 );

	TracePrintf( 0, "[Testing @ iolib.c @ Create]: copied path name\n" );
	openFileTable[newfd].fd = newfd;
	openFileTable[newfd].isOpen = 1;

	//create the file from server
	msg.messageType = CREATE;
	msg.pathname = pathCopy;
	msg.len = length + 1;

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send == ERROR )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Create]: The send status is Error.\n" );
		return ERROR;
	}
	openFileTable[newfd].inode = msg.inode;

	TracePrintf( 0, "[Testing @ iolib.c @ Create]: created the file with inode number: %d\n", msg.inode );
	free( pathCopy );

	return newfd;
}

extern int Read( int fd, void *buf, int size )
{
	if( isFileDescriptorLegal( fd ) != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.c @ Read]: The fd %d is not legal\n", fd );
		return ERROR;
	}
	struct Message msg;
	msg.messageType = READ;
	msg.inode = openFileTable[fd].inode;
	msg.len = openFileTable[fd].currentPos;
	msg.buf = buf;
	msg.size = size;

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Read]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Write( int fd, void *buf, int size )
{
	if( isFileDescriptorLegal( fd ) != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.c @ Write]: The fd %d is not legal\n", fd );
		return ERROR;
	}
	struct Message msg;
	msg.messageType = WRITE;
	msg.inode = openFileTable[fd].inode;
	msg.len = openFileTable[fd].currentPos;
	msg.buf = buf;
	msg.size = size;

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.c @ Write]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Seek( int fd, int offset, int whence )
{
	if(isFileDescriptorLegal(fd) == ERROR)
	{
		TracePrintf(0, "[Error @ iolib.c @ Seek]: The fd: %d is not legal, MAX_OPEN_FILES: %d\n", fd, MAX_OPEN_FILES);
		return ERROR;
	}

	struct OpenFileEntry *ofe = &openFileTable[fd]; 
	int inodeNum = ofe -> inode;
	int currentPos = ofe -> currentPos;

	struct Message msg;
	msg.messageType = SEEK;
	msg.inode = inodeNum;
	msg.len = currentPos;

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Seek]: The send status is Error.\n" );
		return ERROR;
	}

	int fileSize = msg.size;
	TracePrintf(200, "[Testing @ iolib.h @ Seek]: The file size is %d\n", fileSize);

	if(whence == SEEK_SET)
	{
		if(offset < 0 || offset > fileSize)
		{
			TracePrintf( 0, "[Error @ iolib.h @ Seek]: The offset: %d is Error when fileSize: %d, whence: SEEK_SET.\n", offset, fileSize );
			return ERROR;
		}
		ofe -> currentPos = offset;
	}
	else if(whence == SEEK_CUR)
	{
		int newPos = currentPos + offset;
		if(newPos < 0 || newPos > fileSize)
		{
			TracePrintf( 0, "[Error @ iolib.h @ Seek]: The offset: %d is Error when fileSize: %d, whence: SEEK_CUR, newPos: %d.\n", offset, fileSize, newPos );
			return ERROR;
		}
		ofe -> currentPos = newPos; 
	}
	else if(whence == SEEK_END)
	{
		if(offset > 0 || offset > fileSize)
		{
			TracePrintf( 0, "[Error @ iolib.h @ Seek]: The offset: %d is Error when fileSize: %d, whence: SEEK_END.\n", offset, fileSize );
			return ERROR;
		}
		ofe -> currentPos = fileSize + offset;
	}
	else
	{
		TracePrintf( 0, "[Error @ iolib.h @ Seek]: The whence is Error: %d.\n", whence );
		return ERROR;
	}
	
	TracePrintf( 0, "[Testing @ iolib.h @ Seek]: Finish Seeking: offset: %d, fileSize: %d, whence: %d, newPos: %d = %d.\n", offset, fileSize, whence, ofe -> currentPos, openFileTable[fd].currentPos );

	return 0;
}

extern int Link( char *oldname, char *newname )
{
	struct Message msg;
	msg.messageType = LINK;
	msg.pathname = oldname;
	msg.len = strlen( oldname );
	msg.buf = newname;
	msg.size = strlen( newname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Link]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Unlink( char *pathname )
{
	struct Message msg;
	msg.messageType = UNLINK;
	msg.pathname = pathname;
	msg.len = strlen( pathname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Unlink]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int SymLink( char *oldname, char *newname )
{
	struct Message msg;
	msg.messageType = SYMLINK;
	msg.pathname = oldname;
	msg.len = strlen( oldname );
	msg.buf = newname;
	msg.size = strlen( newname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ SymLink]: The send status is Error.\n" );
		return ERROR;
	}

	return 0;
}

extern int ReadLink( char *pathname, char *buf, int size )
{
	struct Message msg;
	msg.messageType = READLINK;
	msg.pathname = pathname;
	msg.len = strlen( pathname );
	msg.buf = buf;
	msg.size = strlen( buf );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ ReadLink]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int MkDir( char *pathname )
{
	struct Message msg;
	msg.messageType = MKDIR;
	msg.pathname = pathname;
	msg.len = strlen( pathname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ MkDir]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int ChDir( char *pathname )
{
	struct Message msg;
	msg.messageType = CHDIR;
	msg.pathname = pathname;
	msg.len = strlen( pathname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ ChDir]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int RmDir( char *pathname )
{
	struct Message msg;
	msg.messageType = RMDIR;
	msg.pathname = pathname;
	msg.len = strlen( pathname );

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ RmDir]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Stat( char *pathname, struct Stat *statbuf )
{
	struct Message msg;
	msg.messageType = STAT;
	msg.pathname = pathname;
	msg.len = strlen( pathname );

	msg.size = sizeof(statbuf); //unfinished

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Stat]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Sync()
{
	struct Message msg;
	msg.messageType = SYNC;

	int send = Send( (void *) &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Sync]: The send status is Error.\n" );
		return ERROR;
	}
	return 0;
}

extern int Shutdown()
{
	struct Message msg;
	msg.messageType = SHUTDOWN;

	int send = Send( &msg, FILE_SERVER );
	if( send != 0 )
	{
		TracePrintf( 0, "[Error @ iolib.h @ Shutdown]: The send status is Error.\n" );
		return ERROR;
	}

	TracePrintf( 500, "[Testing @ iolib.h @ Shutdown]: Reply: messageType(%d)\n", msg.messageType );
	return 0;
}

