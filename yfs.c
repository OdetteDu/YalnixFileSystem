#include <stdio.h>
#include <comp421/yalnix.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

int main(int argc, char **argv)
{
	int registerStatus = Register(FILE_SERVER);
	if(registerStatus != 0)
	{
		TracePrintf(0, "[Error @ yfs.main]: unsuccessfully register the YFS as the FILE_SERVER.\n");
	}

	return 0;
}
