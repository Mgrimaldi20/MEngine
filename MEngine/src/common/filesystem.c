#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"
#include "unzip.h"
#include "zip.h"

#define DEF_FILE_MAP_CAPACITY 512

typedef enum
{
	FILE_FROM_PAK,
	FILE_FROM_DISK
} filesource_t;

typedef struct
{
	filesource_t source;
	char filename[SYS_MAX_PATH];
	union
	{
		unzFile pakfile;
		FILE *diskfile;
	} handle;
} vfile_t;

typedef struct fileentry
{
	char filename[SYS_MAX_PATH];
	char pakfile[SYS_MAX_PATH];
	struct fileentry *next;
} fileentry_t;

typedef struct
{
	size_t numfiles;
	size_t capacity;
	fileentry_t **files;
} filemap_t;

static filemap_t *filemap;

static cvar_t *fsbasepath;
static cvar_t *fssavepath;

static bool initialized;

/*
* Function: HashFileName
* Hashes the name of the file to generate an index, using the FNV-1a algorithm
* 
* 	name: The name of the file
* 
* Returns: The hash value
*/
static size_t HashFileName(const char *name)
{
	size_t hash = 2166136261u;	// initial offset basis, large prime number
	size_t len = Sys_Strlen(name, SYS_MAX_PATH);

	for (size_t i=0; i<len; i++)
	{
		hash ^= (unsigned char)name[i];
		hash *= 16777619;	// FNV prime number
	}

	return(hash & (filemap->capacity - 1));
}

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
* Function: MapFiles
* Maps files from the all the pak files to the file map.
* Files in the paks have priority over files on disk.
* Files in higher numbered paks have priority over lower numbered paks.
* Eg. pak.1.pk has priority over pak.0.pk, etc...
* 
* 	filelist: The list of files to map to the VFS
* 	numfiles: The number of files in the list
* 
* Returns: A boolean if the files were mapped successfully or not
*/
static bool MapFiles(const filedata_t *filelist, const unsigned int numfiles)
{
	for (unsigned int i=0; i<numfiles; i++)
	{
		const char *pakfile = filelist[i].filename;

		unzFile pak = unzOpen(pakfile);
		if (!pak)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to open pak file: %s", pakfile);
			return(false);
		}

		if (unzGoToFirstFile(pak) != UNZ_OK)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to go to the first file in the pak: %s", pakfile);
			unzClose(pak);
			return(false);
		}

		do
		{
			unz_file_info info;

			char filename[SYS_MAX_PATH] = { 0 };
			if (unzGetCurrentFileInfo(pak, &info, filename, SYS_MAX_PATH, NULL, 0, NULL, 0) != UNZ_OK)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to get the current file info in the pak: %s", pakfile);
				unzClose(pak);
				return(false);
			}

			fileentry_t *entry = MemCache_Alloc(sizeof(*entry));
			if (!entry)
			{
				Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for file entry");
				unzClose(pak);
				return(false);
			}

			size_t index = HashFileName(filename);

			entry->next = filemap->files[index];
			filemap->files[index] = entry;

			snprintf(entry->filename, SYS_MAX_PATH, "%s", filename);
			snprintf(entry->pakfile, SYS_MAX_PATH, "%s", pakfile);
		} while (unzGoToNextFile(pak) == UNZ_OK);

		unzClose(pak);
	}

	return(true);
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

	char basepath[SYS_MAX_PATH] = { 0 };

	if (!Cvar_GetString(fsbasepath, basepath))
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get base path from Cvar system");
		return(false);
	}

	unsigned int numfiles = 0;
	filedata_t *pakfiles = FileSys_ListFiles(&numfiles, basepath, "pak.*.pk");
	if (!pakfiles && numfiles)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to create a pak file list");
		return(false);
	}

	if (pakfiles && numfiles)
	{
		filemap = MemCache_Alloc(sizeof(*filemap));
		if (!filemap)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for file map");
			FileSys_FreeFileList(pakfiles);
			return(false);
		}

		filemap->capacity = DEF_FILE_MAP_CAPACITY;
		filemap->numfiles = 0;

		filemap->files = MemCache_Alloc(sizeof(*filemap->files) * filemap->capacity);
		if (!filemap->files)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for file map entries");
			FileSys_FreeFileList(pakfiles);
			MemCache_Free(filemap);
			return(false);
		}

		for (size_t i=0; i<filemap->capacity; i++)
			filemap->files[i] = NULL;

		if (!MapFiles(pakfiles, numfiles))
		{
			Log_WriteSeq(LOG_ERROR, "Failed to map the files sourced from the PAKs");
			FileSys_FreeFileList(pakfiles);
			MemCache_Free(filemap->files);
			MemCache_Free(filemap);
			return(false);
		}

		FileSys_FreeFileList(pakfiles);
	}

	if (!pakfiles && !numfiles)
		Log_WriteSeq(LOG_WARN, "No pak files found in the base path, using regular filesystem");

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

	Log_WriteSeq(LOG_INFO, "Shutting down filesystem");

	if (filemap)
	{
		for (size_t i=0; i<filemap->capacity; i++)
		{
			fileentry_t *current = filemap->files[i];
			while (current)
			{
				fileentry_t *next = current->next;
				MemCache_Free(current);
				current = next;
			}
		}

		MemCache_Free(filemap->files);
		MemCache_Free(filemap);
	}

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
				Log_Write(LOG_WARN, "File path too long: %s/%s", directory, filename);
				continue;
			}

			char filepath[SYS_MAX_PATH] = { 0 };
			snprintf(filepath, SYS_MAX_PATH, "%s/%s", directory, filename);

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
void FileSys_FreeFileList(filedata_t *filelist)
{
	if (filelist)
	{
		MemCache_Free(filelist);
		filelist = NULL;
	}
}
