#include <stdio.h>
#include <stdlib.h>
#include <comp421/iolib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

int main(int argc, char **argv)
{
	printf("Hello, World!\n");
	MkDir("Test");
	MkDir("VerLongTest");
	MkDir("DDDDDDDDD");
	Open("Test");
	Link("Test", "FLYTOSkyyyyyyyyyyyyyyyyyyyyyyyyyyyyyy");
	Link("VerLongTest", "FUCKYOU");
	Unlink("FUCKYOU");
	MkDir("Sweet");
	Unlink("FLYTOSkyyyyyyyyyyyyyyyyyyyyyyyyyyyyy");
	
	Open("DDDDDDDDD");
	ChDir("Test");
	MkDir("FUCKLIFE");
	ChDir("FUCKLIFE");
	ChDir("..");
	RmDir("VerLongTest");
	RmDir("Sweet");
	
	Exit(0);
		  
}

