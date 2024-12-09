#include <stdio.h>
#include <stdlib.h>
#include "sys/sys.h"
#include "common.h"
#include "unzip.h"

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
	initialized = true;

	// TODO: add init code here, this will read all the .pk files in the directory and load them into memory
	// .pk files will overwrite each other if the number is higher, so the last one loaded will overwrite the previous ones
	// this also means the below functions dont have to open zip files, they can just read from memory
	// non *PAK* functions will still read from disk and dont need to be initialized

	return(false);
}

void FileSys_Shutdown(void)
{
	if (!initialized)
		return;

	// TODO: add shutdown code here, this will free all the memory used by the .pk files

	initialized = false;
}

bool FileSys_FileExistsInPAK(const char *filename)
{
	if (!filename)
		return(false);

	unzFile pakfile = unzOpen64(filename);
	if (!pakfile)
		return(false);

	if (unzLocateFile(pakfile, filename, 0) != UNZ_OK)
	{
		unzClose(pakfile);
		return(false);
	}

	unzClose(pakfile);
	return(true);
}

filedata_t *FileSys_ListFilesInPAK(unsigned int *numfiles, const char *directory, const char *filter)	// uses same free FileSys_FreeFileList function
{
	if (!numfiles || !directory || !filter)
		return(NULL);

	unzFile pakfile = unzOpen64(directory);
	if (!pakfile)
		return(NULL);

	if (unzGoToFirstFile(pakfile) != UNZ_OK)
	{
		unzClose(pakfile);
		return(NULL);
	}

	unsigned int filecount = 0;

	do
	{
		char filename[SYS_MAX_PATH] = { 0 };
		unz_file_info info;

		if (unzGetCurrentFileInfo(pakfile, &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
			continue;

		if (strstr(filename, filter))
			filecount++;

	} while (unzGoToNextFile(pakfile) == UNZ_OK);

	if (filecount == 0)
	{
		unzClose(pakfile);
		return(NULL);
	}

	filedata_t *filelist = MemCache_Alloc(filecount * sizeof(*filelist));
	if (!filelist)
	{
		unzClose(pakfile);
		return(NULL);
	}

	if (unzGoToFirstFile(pakfile) != UNZ_OK)
	{
		unzClose(pakfile);
		MemCache_Free(filelist);
		return(NULL);
	}

	unsigned int index = 0;

	do
	{
		char filename[SYS_MAX_PATH] = { 0 };
		unz_file_info64 info;

		if (unzGetCurrentFileInfo64(pakfile, &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
			continue;

		if (PathMatchSpec(filename, filter))
		{
			filedata_t *file = &filelist[index];

			struct tm timeinfo = { 0 };
			timeinfo.tm_sec = info.tmu_date.tm_sec;
			timeinfo.tm_min = info.tmu_date.tm_min;
			timeinfo.tm_hour = info.tmu_date.tm_hour;
			timeinfo.tm_mday = info.tmu_date.tm_mday;
			timeinfo.tm_mon = info.tmu_date.tm_mon;
			timeinfo.tm_year = info.tmu_date.tm_year - 1900;
			timeinfo.tm_isdst = -1;

			time_t filetime = mktime(&timeinfo);

			snprintf(file->filename, SYS_MAX_PATH, "%s", filename);
			file->filesize = info.uncompressed_size;
			file->atime = filetime;
			file->mtime = filetime;
			file->ctime = filetime;

			index++;
		}
	} while (unzGoToNextFile(pakfile) == UNZ_OK);

	*numfiles = filecount;
	unzClose(pakfile);

	return(filelist);
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
			char filepath[SYS_MAX_PATH] = { 0 };
			snprintf(filepath, SYS_MAX_PATH, "%s/%s", directory, filename);

			Sys_Stat(filepath, &filelist[index]);
			index++;
		}
	}

	Sys_CloseDir(dir);
	*numfiles = filecount;

	return(filelist);
}

void FileSys_FreeFileList(filedata_t *filelist)
{
	if (!filelist)
		return;

	MemCache_Free(filelist);
	filelist = NULL;
}
