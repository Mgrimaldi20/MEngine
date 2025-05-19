#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"

static cvar_t *fsbasepath;
static cvar_t *fssavepath;

static bool initialized;

/*
* Function: PathMatchSpec
* Matches a path to a filter, can also be a file type
* 
*	path: The full path or filename to match
*	filter: The filter to match against, wildcard as *
* 
* Returns: A boolean if the path matches the filter
*/
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

/*
* Function: FileSys_Init
* Initializes the filesystem, if no PAK files are found, the filesystem will use the regular disk instead of the PAKs
* 
* Returns: A boolean if initialization was successful or not
*/
bool FileSys_Init(void)
{
	if (initialized)
		return(true);

	fsbasepath = Cvar_RegisterString("fs_basepath", "", CVAR_FILESYSTEM | CVAR_READONLY, "The base path for the engine. Path to the installation");
	fssavepath = Cvar_RegisterString("fs_savepath", "save", CVAR_FILESYSTEM, "The path to the games save files, relative to the base path");

	initialized = true;

	return(true);
}

/*
* Function: FileSys_Shutdown
* Shuts down the filesystem
*/
void FileSys_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down filesystem");

	initialized = false;
}

/*
* Function: FileSys_FileExists
* Checks if a file exists on the filesystem
* 
* 	filename: The filename to check
* 
* Returns: A boolean if the file exists or not
*/
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

/*
* Function: FileSys_ListFiles
* Lists all files in a directory with a given filter
* 
* 	numfiles: The number of files found
* 	directory: The directory to search
* 	filter: The filter to apply to the file list
* 
* Returns: A list of filedata_t structs
*/
filedata_t *FileSys_ListFiles(unsigned int *numfiles, const char *directory, const char *filter)
{
	void *dir = Sys_OpenDir(directory);
	if (!dir)
	{
		*numfiles = 0;
		return(NULL);
	}

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

	*numfiles = filecount;

	filedata_t *filelist = MemCache_Alloc(filecount * sizeof(*filelist));
	if (!filelist)
	{
		Sys_CloseDir(dir);
		return(NULL);
	}

	Sys_CloseDir(dir);	// reset the directory pointer
	dir = NULL;

	dir = Sys_OpenDir(directory);
	if (!dir)
	{
		MemCache_Free(filelist);
		return(NULL);
	}

	unsigned int index = 0;
	while (Sys_ReadDir(dir, filename, SYS_MAX_PATH))
	{
		if (PathMatchSpec(filename, filter))
		{
			size_t dirlen = Sys_Strlen(directory, SYS_MAX_PATH) + 1;
			size_t filelen = Sys_Strlen(filename, SYS_MAX_PATH) + 1;

			if ((dirlen + filelen) > SYS_MAX_PATH)
			{
				Log_Writef(LOG_WARN, "File path too long: %s/%s", directory, filename);
				continue;
			}

			char filepath[SYS_MAX_PATH + 1] = { 0 };
			snprintf(filepath, SYS_MAX_PATH + 1, "%s/%s", directory, filename);

			Sys_Stat(filepath, &filelist[index]);
			index++;
		}
	}

	Sys_CloseDir(dir);
	return(filelist);
}

/*
* Function: FileSys_FreeFileList
* Frees the file list allocated by FileSys_ListFiles and FileSys_ListFilesInPAK
* 
* 	filelist: The list of filedata_t structs
*/
void FileSys_FreeFileList(filedata_t **filelist)
{
	if (filelist && *filelist)
	{
		MemCache_Free(*filelist);
		*filelist = NULL;
	}
}
