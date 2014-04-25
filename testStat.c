#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	struct Stat *statbuf = malloc(sizeof(struct Stat));
	int fd = Stat("/odette", statbuf);
	TracePrintf(500, "return from seek\n");
	
	Exit(0);
}
