#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	Create("/Test///abc/de/f//gh/i//j/kl//mn/odette");
	
	TracePrintf(500, "return from open\n");
	return 0;
}
