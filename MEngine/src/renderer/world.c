#include <stdio.h>
#include <string.h>
#include "common/common.h"
#include "world.h"

#define WLD_VERSION 1
#define WLD_MAGIC "WLD1"
#define AREA_MAGIC "AREA"

/*
* Functn: ReadExactBytes
* Reads a specified number of bytes from a file into a buffer, with byte size (1)
* 
*	file: The file to read from
*	out: The buffer to read the bytes into
*	num: The number of bytes to read
* 
* Returns: true if the exact number of bytes were read, else false
*/
static bool ReadExactBytes(FILE *file, void *out, size_t num)
{
	return(fread(out, 1, num, file) == num);
}

/*
* Functn: World_Load
* Loads a .wld file from disk and returns a pointer to a new world_t structure.
* This function only loads the world metadata, everything else is loaded on demand
* 
*	filename: The full path to the .wld file to load
* 
* Returns: A pointer to a new world_t structure, or NULL if the file could not be loaded
*/
world_t *World_Load(const char *filename)
{
	unsigned int i = 0;

	FILE *file = fopen(filename, "rb");
	if (!file)
	{
		Log_Writef(LOG_ERROR, "Failed to open world file: %s", filename);
		return(NULL);
	}

	world_t *world = MemCache_Alloc(sizeof(*world));
	if (!world)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for world_t structure");
		goto error;
	}

	memset(world, 0, sizeof(*world));

	if (!ReadExactBytes(file, &world->header, sizeof(world->header)) ||
		memcmp(world->header.magic, WLD_MAGIC, WLD_MAGIC_LEN) != 0 ||
		world->header.version != WLD_VERSION)
	{
		Log_Writef(LOG_ERROR, "Failed to read world header file record for file %s | world: [%s, %d] def: [%s, %d]",
			filename,
			world->header.magic,
			world->header.version,
			WLD_MAGIC,
			WLD_VERSION
		);

		goto error;
	}

	world->areas = MemCache_Alloc(sizeof(*world->areas) * world->header.areacount);
	if (!world->areas)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for area array");
		goto error;
	}

	memset(world->areas, 0, sizeof(*world->areas) * world->header.areacount);

	for (i=0; i<world->header.areacount; i++)
	{
		areaheader_t *area = &world->areas[i];
		if (!ReadExactBytes(file, area->magic, sizeof(area->magic)) ||
			memcmp(area->magic, AREA_MAGIC, WLD_MAGIC_LEN) != 0)
		{
			Log_Writef(LOG_ERROR, "Failed to read area header file record for area %d in world %s", i, filename);
			goto error;
		}

		if (!ReadExactBytes(file, &area->areanamelen, sizeof(area->areanamelen)))
		{
			Log_Writef(LOG_ERROR, "Failed to read area name length for area %d in world %s", i, filename);
			goto error;
		}

		area->areaname = MemCache_Alloc(area->areanamelen + 1);
		if (!area->areaname)
		{
			Log_Writef(LOG_ERROR, "Failed to allocate memory for area name in area %d in world %s", i, filename);
			goto error;
		}

		memset(area->areaname, 0, area->areanamelen + 1);

		if (!ReadExactBytes(file, area->areaname, area->areanamelen))
		{
			Log_Writef(LOG_ERROR, "Failed to read area name for area %d in world %s", i, filename);
			goto error;
		}

		area->areaname[area->areanamelen] = '\0';

		if (!ReadExactBytes(file, &area->chunkcount, sizeof(area->chunkcount)))
		{
			Log_Writef(LOG_ERROR, "Failed to read chunk count for area %d (%s) in world %s", i, area->areaname, filename);
			goto error;
		}

		area->chunks = MemCache_Alloc(sizeof(*area->chunks) * area->chunkcount);
		if (!area->chunks)
		{
			Log_Writef(LOG_ERROR, "Failed to allocate memory for chunk array in area %d (%s) in world %s", i, area->areaname, filename);
			goto error;
		}

		memset(area->chunks, 0, sizeof(*area->chunks) * area->chunkcount);

		if (!ReadExactBytes(file, area->chunks, sizeof(*area->chunks) * area->chunkcount))
		{
			Log_Writef(LOG_ERROR, "Failed to read chunk data for area %d (%s) in world %s", i, area->areaname, filename);
			goto error;
		}
	}

	world->filehandle = file;
	return(world);

error:
	if (world && world->areas)
	{
		for (unsigned int j=0; j<i; j++)
		{
			areaheader_t *area = &world->areas[j];

			if (area->areaname)
				MemCache_Free(area->areaname);

			if (area->chunks)
				MemCache_Free(area->chunks);
		}

		MemCache_Free(world->areas);
	}

	if (world)
		MemCache_Free(world);

	if (file)
		fclose(file);

	return(NULL);
}

/*
* Functn: World_Unload
* Unloads a world_t structure and frees all memory associated with it
* 
*	world: A pointer to the world_t structure to unload
*/
void World_Unload(world_t *world)
{
	if (!world)
		return;

	for (unsigned int i=0; i<world->header.areacount; i++)
	{
		areaheader_t *area = &world->areas[i];

		if (area->areaname)
			MemCache_Free(area->areaname);

		if (area->chunks)
			MemCache_Free(area->chunks);
	}

	MemCache_Free(world->areas);
	fclose(world->filehandle);
	MemCache_Free(world);
}
