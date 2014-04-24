#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
//	int fd =Create("/Test///abc/de/f//gh/i//j/kl//mn/odette");
//	int fd2 = Open("/Test///abc/de/f//gh/i//j/kl//mn/odette");
//	MkDir("/Test");
//	MkDir("Test/test2");
//	ChDir("/Test/abc/fg");
//	Close(fd);
//	Close(fd2);
	int fd = Create("/odette");
	TracePrintf(500, "return from open\n");
	Seek(fd, 0, SEEK_SET);
	TracePrintf(500, "return from seek\n");
	
//	return 0;
	Exit(0);
}
