#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "unzip.h"

bool FileSys_Init(void)
{
	return(false);
}

void FileSys_Shutdown(void)
{
}

bool FileSys_FileExists(const char *filename)
{
	FILE *file = fopen(filename, "r");
	if (!file)
		return(false);

	fclose(file);
	return(true);
}

size_t FileSys_FileSize(const char *filename)
{
	return(0);
}

time_t FileSys_FileTimeStamp(const char *filename)
{
	return(0);
}

unsigned int FileSys_CountFiles(const char *directory, const char *filter)
{
	return(0);
}

filedata_t *FileSys_ListFiles(const char *directory, const char *filter, unsigned int numfiles)		// if numfiles is 0, all files will be listed
{
	return(NULL);
}

void FileSys_FreeFileList(filedata_t *filelist)
{
	free(filelist);
	filelist = NULL;
}
