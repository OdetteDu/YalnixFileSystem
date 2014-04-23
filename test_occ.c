#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	int fd =Create("odette");
	int fd2 = Open("odette");
	Close(fd);
	Close(fd2);
	
	TracePrintf(500, "return from open\n");
//	return 0;
	Exit(0);
		  
}

