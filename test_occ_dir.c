#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
//	int fd =Create("/odette");
	MkDir("Test");
	MkDir("/Test///////////////test2");
	int fd2 = Open("Test/test2");
//	Close(fd);
	Close(fd2);
	ChDir("Test");

	TracePrintf(500, "return from open\n");
//	return 0;
	Exit(0);
		  
}

