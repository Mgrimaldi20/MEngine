#pragma once

#define WLD_MAGIC_LEN 4

typedef struct
{
	char magic[WLD_MAGIC_LEN];
	unsigned int version;
	unsigned int areacount;
} worldheader_t;

typedef struct
{
	int chunkx;
	int chunky;
	unsigned long long offset;
} chunkref_t;

typedef struct
{
	char magic[WLD_MAGIC_LEN];
	unsigned short areanamelen;
	char *areaname;
	unsigned int chunkcount;
	chunkref_t *chunkrefs;
} areaheader_t;
