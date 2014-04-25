#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	int fd =Create("/odette");
	//int fd2 = Open("odette");
	int fd2 = Create("this is a very long path name");
	//MkDir("odette");
	int fd3 = Create("bing");
	Close(fd);
	Close(fd2);
	Close(fd3);
	
//	Link("odette", "link_it");
	TracePrintf(500, "return from open\n");
//	return 0;
	Exit(0);
		  
}

