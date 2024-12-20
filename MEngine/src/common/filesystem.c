#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"
#include "unzip.h"
#include "zip.h"

#define CHUNK_SIZE 0xFFFFFFFF	// default zip size is 4GB, so read in 4GB chunks

static filedata_t *pakfilelist;
static unzFile *pakfiles;
static unsigned int pakfilecount;
static unzFile finalpakfile;

static const char *finalpkname = "final.pk";

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

static bool ReadFileInChunks(unzFile file, char *buffer, ZPOS64_T size)
{
	ZPOS64_T remaining = size;
	char *current = buffer;

	while (remaining > 0)
	{
		uint32_t toread = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : (uint32_t)remaining;
		int bytesread = unzReadCurrentFile(file, current, toread);

		if (bytesread < 0)
			return(false);

		current += bytesread;
		remaining -= bytesread;
	}

	return(true);
}

static bool WriteFileInChunks(zipFile file, const char *buffer, ZPOS64_T size)
{
	ZPOS64_T remaining = size;
	const char *current = buffer;

	while (remaining > 0)
	{
		uint32_t towrite = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : (uint32_t)remaining;
		int byteswritten = zipWriteInFileInZip(file, current, towrite);

		if (byteswritten < 0)
			return(false);

		current += byteswritten;
		remaining -= byteswritten;
	}

	return(true);
}

bool FileSys_Init(void)
{
	return(true);	// just do this for now

	/*pakfilelist = FileSys_ListFiles(&pakfilecount, ".", "pak.*.pk");
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

			for (unsigned int j=0; j<i; j++)
			{
				if (pakfiles[j])
					unzClose(pakfiles[j]);
			}

			return(false);
		}
	}

	// create the final.pk file and add the files from pak.0.pk ... pak.*.pk to it, overwriting the previous files if they are the same
	zipFile outpakfile = zipOpen64(finalpkname, APPEND_STATUS_CREATE);
	if (!outpakfile)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open the final PAK file for writing");
		for (unsigned int i=0; i<pakfilecount; i++)
		{
			if (pakfiles[i])
				unzClose(pakfiles[i]);
		}

		MemCache_Free(pakfiles);
		FileSys_FreeFileList(pakfilelist);
		return(false);
	}

	for (unsigned int i=0; i<pakfilecount; i++)
	{
		if (unzGoToFirstFile(pakfiles[i]) != UNZ_OK)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to go to the first file in PAK file: %s", pakfilelist[i].filename);
			zipClose(outpakfile, NULL);
			for (unsigned int j=0; j<pakfilecount; j++)
			{
				if (pakfiles[j])
					unzClose(pakfiles[j]);
			}

			MemCache_Free(pakfiles);
			FileSys_FreeFileList(pakfilelist);
			return(false);
		}

		do
		{
			char filename[SYS_MAX_PATH] = { 0 };
			unz_file_info64 info;
			zip_fileinfo zipinfo;

			if (unzGetCurrentFileInfo64(pakfiles[i], &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
				continue;

			if (unzOpenCurrentFile(pakfiles[i]) != UNZ_OK)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to open the current file in PAK file: %s", pakfilelist[i].filename);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}

			char *filedata = MemCache_Alloc(info.uncompressed_size);
			if (!filedata)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for file data in PAK file: %s", pakfilelist[i].filename);
				unzCloseCurrentFile(pakfiles[i]);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}

			if (!ReadFileInChunks(pakfiles[i], filedata, info.uncompressed_size))
			{
				Log_WriteSeq(LOG_ERROR, "Failed to read the current file in PAK file: %s", pakfilelist[i].filename);
				MemCache_Free(filedata);
				unzCloseCurrentFile(pakfiles[i]);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}

			if (zipOpenNewFileInZip(outpakfile, filename, &zipinfo, NULL, 0, NULL, 0, NULL, 0, 0) != ZIP_OK)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to open a new file in the final PAK file: %s", finalpkname);
				MemCache_Free(filedata);
				unzCloseCurrentFile(pakfiles[i]);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}

			if (!WriteFileInChunks(outpakfile, filedata, info.uncompressed_size))
			{
				Log_WriteSeq(LOG_ERROR, "Failed to write a file in the final PAK file: %s", finalpkname);
				MemCache_Free(filedata);
				unzCloseCurrentFile(pakfiles[i]);
				zipCloseFileInZip(outpakfile);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}

			MemCache_Free(filedata);

			if (unzCloseCurrentFile(pakfiles[i]) != UNZ_OK)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to close the current file in PAK file: %s", pakfilelist[i].filename);
				zipClose(outpakfile, NULL);
				for (unsigned int j=0; j<pakfilecount; j++)
				{
					if (pakfiles[j])
						unzClose(pakfiles[j]);
				}

				MemCache_Free(pakfiles);
				FileSys_FreeFileList(pakfilelist);
				return(false);
			}
		} while (unzGoToNextFile(pakfiles[i]) == UNZ_OK);

		unzClose(pakfiles[i]);
	}

	if (zipClose(outpakfile, NULL) != ZIP_OK)		// must be closed before it can be read... its okay if unzClose fails as the program will exit anyway
	{
		Log_WriteSeq(LOG_ERROR, "Failed to close the final PAK file: %s", finalpkname);
		for (unsigned int i=0; i<pakfilecount; i++)
		{
			if (pakfiles[i])
				unzClose(pakfiles[i]);
		}

		MemCache_Free(pakfiles);
		FileSys_FreeFileList(pakfilelist);
		return(false);
	}

	finalpakfile = unzOpen64(finalpkname);	// open the final PAK file for reading now
	if (!finalpakfile)
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

	initialized = true;

	return(true);*/
}

void FileSys_Shutdown(void)
{
	if (!initialized)
		return;

	if (finalpakfile)
		unzClose(finalpakfile);

	remove("final.pk");		// the final PAK file is only temporary, so remove it after the engine quits

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

	if (unzLocateFile(finalpakfile, filename, 0) != UNZ_OK)
		return(false);

	return(true);
}

filedata_t *FileSys_ListFilesInPAK(unsigned int *numfiles, const char *filter)	// uses same free FileSys_FreeFileList function
{
	if (!numfiles || !filter)
		return(NULL);

	if (unzGoToFirstFile(finalpakfile) != UNZ_OK)
		return(NULL);

	unsigned int filecount = 0;

	do
	{
		char filename[SYS_MAX_PATH] = { 0 };
		unz_file_info info;

		if (unzGetCurrentFileInfo(finalpakfile, &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
			continue;

		if (strstr(filename, filter))
			filecount++;

	} while (unzGoToNextFile(finalpakfile) == UNZ_OK);

	if (filecount == 0)
		return(NULL);

	filedata_t *filelist = MemCache_Alloc(filecount * sizeof(*filelist));
	if (!filelist)
		return(NULL);

	if (unzGoToFirstFile(finalpakfile) != UNZ_OK)
	{
		MemCache_Free(filelist);
		return(NULL);
	}

	unsigned int index = 0;

	do
	{
		char filename[SYS_MAX_PATH] = { 0 };
		unz_file_info64 info;

		if (unzGetCurrentFileInfo64(finalpakfile, &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
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
	} while (unzGoToNextFile(finalpakfile) == UNZ_OK);

	*numfiles = filecount;

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
