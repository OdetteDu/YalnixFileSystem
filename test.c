#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
//	int fd = Create("/odette");
	int fd = Open("/odette");
//	TracePrintf(500, "return from open\n");
//	char *s1 = "123456789";
//	Write(fd, s1, strlen(s1)); 
//	Seek(fd, 2, SEEK_SET);
//	char *s2 = "abcdefghijklmnopqrstuvwxyz";
//	Write(fd, s2, strlen(s2));
    char *s3 = malloc(sizeof(char) * 5);
	Read(fd, s3, 5);
	Seek(fd, 8, SEEK_SET);
    char *s4 = malloc(sizeof(char) * 10);
	Read(fd, s4, 10);
	TracePrintf(500, "return from seek\n");
	Sync();
	
	Exit(0);
}
