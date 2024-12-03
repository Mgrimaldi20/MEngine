#include <stdio.h>
#include <stdlib.h>
#include "sys/sys.h"
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
	return(Sys_Stat(filename).filesize);
}

time_t FileSys_FileTimeLastWrite(const char *filename)
{
	return(Sys_Stat(filename).lastwritetime);
}

unsigned int FileSys_CountFiles(const char *directory, const char *filter)
{
	(const char *)directory;
	(const char *)filter;
	return(0);
}

filedata_t *FileSys_ListFiles(const char *directory, const char *filter, unsigned int numfiles)		// if numfiles is 0, all files will be listed
{
	(const char *)directory;
	(const char *)filter;
	(const unsigned int)numfiles;
	return(NULL);
}

void FileSys_FreeFileList(filedata_t *filelist)
{
	free(filelist);
	filelist = NULL;
}
