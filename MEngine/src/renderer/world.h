/*
* The engines .wld file format specification.
* This file (world.h) contains the structures used to represent the world data following the .wld format.
* 
* Overview:
* The .wld file format is a binary file formatwhich contains all information needed to represent a 3D world and contains:
*	- A global header for the world.
* 	- Area headers for each area in the world or for a level based system.
*	- Each area contains a list of chunk references, which point to the actual chunks of the area.
*	- Each chunk contains a header, a list of sectors, and a list of entities.
* 
* This file format supports:
*	- Open worlds with multiple areas.
*	- Chunked loading and rendering of world data to optimize memory usage and performance.
*	- Interior and exterior world representation using different areas.
*	- Fast lookup of chunks based upon their chunk references and offset in the file.
* 
* File Format:
* World Header:
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | char[4]        | magic       | Magic: "WLD1" to identify the file format
* 4       | uint32_t       | version     | Version of the world format
* 8       | uint32_t       | areacount   | Number of areas in the world
* 
* Area Header:
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | char[4]        | magic       | Magic: "AREA" to identify the area format
* 4       | uint16_t       | areanamelen | Length of the area name (length is variable, up to 255 characters) (L)
* 6	      | char[]         | areaname    | Name of the area (variable length, up to 255 characters) (L)
* 6+L     | uint32_t       | chunkcount  | Number of chunks in the area (C)
* 6+L+4*C | chunkref_t[]   | chunks      | Array of chunk references
* 
* Chunk Ref (Chunk Reference):
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | uint32_t       | posx        | X position of the chunk in the world
* 4       | uint32_t       | posy        | Y position of the chunk in the world
* 8       | size_t         | offset      | Offset in the file where the chunk data starts
* 16      | size_t         | size        | Size of the chunk data in bytes
* 
* Chunk Header:
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | uint32_t       | posx        | X position of the chunk in the world
* 4       | uint32_t       | posy        | Y position of the chunk in the world
* 8       | uint32_t       | sectorcount | Number of sectors in the chunk
* 12      | uint32_t       | entitycount | Number of entities in the chunk
* 
* Sector:
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | worldpoint_t[] | points      | Array of points defining the sector (P)
* 0*P+12  | uint32_t       | textureid   | Texture ID used for the sector
* 0*P+16  | uint32_t       | flags       | Flags for the sector (e.g., solid, transparent, etc.)
* 
* Entity:
*  Offset |      Type      |    Field    | Description
* --------|----------------|-------------|------------
* 0       | worldpoint_t   | position    | Position of the entity in the world
* 12      | uint32_t       | entityid    | Unique ID of the entity
* 
* Notes:
*	-The size of the worldpoint_t structure is 12 bytes (3 integers (X, Y, Z), each 4 bytes).
*/

#pragma once

#include <stddef.h>
#include <stdbool.h>

#define WLD_MAGIC_LEN 4

typedef struct
{
	int x;			// east-west axis
	int y;			// elevation axis
	int z;			// north-south axis
} worldpoint_t;

typedef struct
{
	worldpoint_t *points;
	unsigned int textureid;
	unsigned int flags;
} worldsector_t;

typedef struct
{
	worldpoint_t position;
	unsigned int entityid;
} worldentity_t;

typedef struct
{
	char magic[WLD_MAGIC_LEN];
	unsigned int version;
	unsigned int areacount;
} worldheader_t;

typedef struct
{
	unsigned int posx;
	unsigned int posy;
	size_t offset;
	size_t size;
} chunkref_t;

typedef struct
{
	char magic[WLD_MAGIC_LEN];
	unsigned short areanamelen;
	char *areaname;
	unsigned int chunkcount;
	chunkref_t *chunks;
} areaheader_t;

typedef struct
{
	unsigned int posx;				// x position in the world
	unsigned int posy;				// y position in the world
	unsigned int sectorcount;
	unsigned int entitycount;
} chunkheader_t;

typedef struct
{
	chunkheader_t header;
	worldsector_t *sectors;
	worldentity_t *entities;
	bool loaded;
} worldchunk_t;

typedef struct
{
	worldheader_t header;
	areaheader_t *areas;
	void *filehandle;
} world_t;
