#pragma once

#include <stdbool.h>

#define SYS_MAX_PATH 260

typedef enum
{
	LOG_INFO = 0,
	LOG_WARN,
	LOG_ERROR
} logtype_t;

typedef enum
{
	CVAR_INT = 0,
	CVAR_FLOAT,
	CVAR_STRING,
	CVAR_BOOL
} cvartype_t;

typedef enum
{
	CVAR_NONE = 0,
	CVAR_ARCHIVE = 1 << 0,
	CVAR_READONLY = 1 << 1,
	CVAR_RENDERER = 1 << 2,
	CVAR_SYSTEM = 1 << 3,
	CVAR_GAME = 1 << 4
} cvarflags_t;

typedef struct cvar cvar_t;		// opaque type to cvar struct, only access through CVar_ functions

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
	void (*Log_Write)(logtype_t type, const char *msg, ...);			// write a log message MT
	void (*Log_WriteSeq)(logtype_t type, const char *msg, ...);			// write a log message sequentially
	void (*Log_WriteLargeSeq)(logtype_t type, const char *msg, ...);	// write a log over the max log length, will allocte heap memory

	// memory cache allocator and manager
	void *(*MemCache_Alloc)(size_t size);
	void (*MemCache_Free)(void *ptr);
	void (*MemCache_Reset)(void);			// reset memory cache, will free all allocated memory and NULL all pointers
	size_t(*MemCahce_GetMemUsed)(void);
	void (*MemCache_Dump)(void);			// dump memory cache stats to log, mostly for debugging purposes

	// cvar system
	void (*CVar_ListAllCVars)(void);		// will write all cvars to the log, mostly for debugging purposes

	cvar_t *(*CVar_Find)(const char *name);

	cvar_t *(*CVar_RegisterString)(const char *name, const char *value, const cvartype_t type, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterInt)(const char *name, const int value, const cvartype_t type, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterFloat)(const char *name, const float value, const cvartype_t type, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterBool)(const char *name, const bool value, const cvartype_t type, const unsigned long long flags, const char *description);

	char *(*CVar_GetString)(cvar_t *cvar);
	int *(*CVar_GetInt)(cvar_t *cvar);
	float *(*CVar_GetFloat)(cvar_t *cvar);
	bool *(*CVar_GetBool)(cvar_t *cvar);
	void (*CVar_SetString)(cvar_t *cvar, const char *value);

	void (*CVar_SetInt)(cvar_t *cvar, const int value);
	void (*CVar_SetFloat)(cvar_t *cvar, const float value);
	void (*CVar_SetBool)(cvar_t *cvar, const bool value);
} mservices_t;
