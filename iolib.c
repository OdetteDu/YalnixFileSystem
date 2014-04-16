#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>

#include "global.h"

extern int Open(char *pathname)
{
	return 0;	
}

extern int Close(int fd)
{
	return 0;	
}

extern int Create(char *pathname)
{
	return 0;	
}

extern int Read(int fd, void *buf, int size)
{
	return 0;	
}

extern int Write(int fd, void *buf, int size)
{
	  
	return 0;	
}

extern int Seek(int fd, int offset, int whence)
{
	return 0;	
}

extern int Link(char *oldname, char *newname)
{
	return 0;	
}

extern int Unlink(char *pathname)
{
	return 0;	
}

extern int SymLink(char *oldname, char *newname)
{
	  
	return 0;	
}

extern int ReadLink(char *pathname, char *buf, int len)
{
	return 0;	
}

extern int MkDir(char *pathname)
{
	return 0;	
}

extern int RmDir(char *pathname)
{
	return 0;
}

extern int Stat(char *pathname, struct Stat *statbuf)
{
	return 0;	
}

extern int Sync(void)
{
	return 0;	
}

extern int Shutdown(void)
{
	return 0;	
}

