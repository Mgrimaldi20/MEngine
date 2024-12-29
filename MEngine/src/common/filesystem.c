#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"
#include "unzip.h"
#include "zip.h"

static cvar_t *fs_basepath;
static cvar_t *fs_savepath;

static bool initialized;

static bool PathMatchSpec(const char *path, const char *filter)
{
	while (*filter && *path)
	{
		if (*filter == '*')
		{
			filter++;
			if (!*filter)
				return(true);

			while (*path)
			{
				if (PathMatchSpec(filter, path))
					return(true);

				path++;
			}
		}
		
		else if (*filter != *path)
			return(false);

		filter++;
		path++;
	}

	return(!*filter && !*path);
}

bool FileSys_Init(void)
{
	if (initialized)
		return(true);

	fs_basepath = CVar_RegisterString("fs_basepath", ".", CVAR_ARCHIVE | CVAR_READONLY, "The base path for the engine. Path to the installation");
	fs_savepath = CVar_RegisterString("fs_savepath", "save", CVAR_ARCHIVE, "The path to the games save files");

	initialized = true;

	return(true);
}

void FileSys_Shutdown(void)
{
	if (!initialized)
		return;

	initialized = false;
}

bool FileSys_FileExistsInPAK(const char *filename)
{
	if (!filename)
		return(false);

	return(true);
}

filedata_t *FileSys_ListFilesInPAK(unsigned int *numfiles, const char *filter)	// uses same free FileSys_FreeFileList function
{
	if (!numfiles || !filter)
		return(NULL);

	*numfiles = 0;
	return(NULL);
}

bool FileSys_FileExists(const char *filename)
{
	if (!filename)
		return(false);

	FILE *file = fopen(filename, "r");
	if (!file)
		return(false);

	fclose(file);
	return(true);
}

filedata_t *FileSys_ListFiles(unsigned int *numfiles, const char *directory, const char *filter)	// also outputs the number of files found
{
	void *dir = Sys_OpenDir(directory);
	if (!dir)
		return(NULL);

	unsigned int filecount = 0;
	char filename[SYS_MAX_PATH] = { 0 };

	while (Sys_ReadDir(dir, filename, SYS_MAX_PATH))
	{
		if (PathMatchSpec(filename, filter))
			filecount++;
	}

	if (filecount == 0)
	{
		Sys_CloseDir(dir);
		*numfiles = 0;
		return(NULL);
	}

	filedata_t *filelist = MemCache_Alloc(filecount * sizeof(*filelist));
	if (!filelist)
	{
		Sys_CloseDir(dir);
		*numfiles = 0;
		return(NULL);
	}

	Sys_CloseDir(dir);
	dir = Sys_OpenDir(directory);

	unsigned int index = 0;
	while (Sys_ReadDir(dir, filename, SYS_MAX_PATH))
	{
		if (PathMatchSpec(filename, filter))
		{
			size_t dirlen = strlen(directory);
			size_t filelen = strlen(filename);

			if ((dirlen + 1 + filelen + 1) <= SYS_MAX_PATH)
			{
				char filepath[SYS_MAX_PATH] = { 0 };
				snprintf(filepath, SYS_MAX_PATH, "%s/%s", directory, filename);

				Sys_Stat(filepath, &filelist[index]);
				index++;
			}

			Log_Write(LOG_WARN, "File path too long: %s/%s", directory, filename);
			continue;
		}
	}

	Sys_CloseDir(dir);
	*numfiles = filecount;

	return(filelist);
}

void FileSys_FreeFileList(filedata_t *filelist)
{
	if (filelist)
	{
		MemCache_Free(filelist);
		filelist = NULL;
	}
}
