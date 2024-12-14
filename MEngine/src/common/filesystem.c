#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"
#include "unzip.h"
#include "zip.h"

static filedata_t *pakfilelist;
static unzFile *pakfiles;
static unzFile respakfile;
static unsigned int pakfilecount;

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
	return(true);	// just do this for now

	pakfilelist = FileSys_ListFiles(&pakfilecount, ".", "pak.*.pk");
	if (!pakfilelist)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to find PAK files to load");
		return(false);
	}

	pakfiles = MemCache_Alloc(pakfilecount * sizeof(*pakfiles));
	if (!pakfiles)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for PAK files");
		FileSys_FreeFileList(pakfilelist);
		return(false);
	}

	for (unsigned int i=0; i<pakfilecount; i++)
	{
		pakfiles[i] = unzOpen64(pakfilelist[i].filename);
		if (!pakfiles[i])
		{
			Log_WriteSeq(LOG_ERROR, "Failed to open PAK file: %s", pakfilelist[i].filename);
			MemCache_Free(pakfiles);
			FileSys_FreeFileList(pakfilelist);
			return(false);
		}
	}

	const char *finalpkname = "final.pk";

	respakfile = unzOpen64(finalpkname);		// create this temp PAK file for writing the contents of all other PAK files to
	if (!respakfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open the final PAK file for reading");

		for (unsigned int i=0; i<pakfilecount; i++)
		{
			if (pakfiles[i])
				unzClose(pakfiles[i]);
		}

		MemCache_Free(pakfiles);
		FileSys_FreeFileList(pakfilelist);
		return(false);
	}

	// fill the final PAK file from pak.0.pk ... pak.*.pk overwriting the previous files if they are the same

	initialized = true;

	return(true);
}

void FileSys_Shutdown(void)
{
	if (!initialized)
		return;

	if (respakfile)
		unzClose(respakfile);

	if (pakfiles)
	{
		for (unsigned int i=0; i<pakfilecount; i++)
		{
			if (pakfiles[i])
				unzClose(pakfiles[i]);
		}

		MemCache_Free(pakfiles);
		pakfiles = NULL;
	}

	if (pakfilelist)
	{
		FileSys_FreeFileList(pakfilelist);
		pakfilelist = NULL;
	}

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
	if (filelist)
	{
		MemCache_Free(filelist);
		filelist = NULL;
	}
}
