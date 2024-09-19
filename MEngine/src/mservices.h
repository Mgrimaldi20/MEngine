#pragma once

#define MENGINE_VERSION 1

#define SYS_MAX_PATH 260

typedef enum
{
	LOG_INFO = 0,
	LOG_WARN,
	LOG_ERROR
} logtype_t;

typedef struct
{
	int version;
	char gamename[256];
	char iconpath[260];			// games icon path
	char smiconpath[260];		// games icon path for small icon
} gameservices_t;

typedef struct
{
	int version;

	// logging service
	void (*Log_Write)(logtype_t, const char *, ...);
	void (*Log_WriteSeq)(logtype_t type, const char *msg, ...);
	void (*Log_WriteLargeSeq)(logtype_t type, const char *msg, ...);

	// memory cache allocator and manager
	void *(*MemCache_Alloc)(size_t size);
	void (*MemCache_Free)(void *ptr);
	void (*MemCache_Reset)(void);
	size_t(*MemCahce_GetMemUsed)(void);
	void (*MemCache_Dump)(void);
} mservices_t;
