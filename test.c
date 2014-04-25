#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	int fd = Create("/odette");
	TracePrintf(500, "return from open\n");
	Write(fd, "Hello, World!", 14); 
	Seek(fd, 7, SEEK_SET);
	Write(fd, "Kitty", 6);
	TracePrintf(500, "return from seek\n");
	
	Exit(0);
}
