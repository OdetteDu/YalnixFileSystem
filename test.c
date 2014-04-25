#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	int fd = Create("/odette");
	TracePrintf(500, "return from open\n");
	char *s1 = "Hello, World";
	Write(fd, s1, strlen(s1)); 
	Seek(fd, 7, SEEK_SET);
	char *s2 = "Kitty, you are a really great cat";
	Write(fd, s2, strlen(s2));
	TracePrintf(500, "return from seek\n");
	
	Exit(0);
}
