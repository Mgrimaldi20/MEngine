#include "common.h"
#include "unzip.h"

bool FileSys_Init(void)
{
	return(true);
}

void FileSys_Shutdown(void)
{
}

bool FileSys_FileIsInPak(const char *filename)
{
	(const char *)filename;
	return(true);
}

void FileSys_ListPakFiles(void)
{
}
