#pragma once

#include <stdbool.h>
#include <time.h>

#define SYS_MAX_PATH 260

typedef enum
{
	LOG_INFO = 0,
	LOG_WARN,
	LOG_ERROR
} logtype_t;

typedef enum
{
	CVAR_NONE = 0,
	CVAR_ARCHIVE = 1 << 0,
	CVAR_READONLY = 1 << 1,
	CVAR_RENDERER = 1 << 2,
	CVAR_SYSTEM = 1 << 3,
	CVAR_GAME = 1 << 4
} cvarflags_t;

typedef struct cvar cvar_t;			// opaque type to cvar struct, only access through CVar_ functions
typedef struct thread thread_t;		// opaque type to thread struct, only access through Sys_ thread functions
typedef struct mutex mutex_t;		// opaque type to mutex struct, only access through Sys_ mutex functions
typedef struct condvar condvar_t;	// opaque type to condvar struct, only access through Sys_ condvar functions

typedef struct
{
	int version;
	char gamename[256];
	char iconpath[260];			// games icon path
	char smiconpath[260];		// games icon path for small icon

	bool (*Init)(void);			// initialize game code, called once at startup
	void (*Shutdown)(void);		// shutdown game code, called once at shutdown
	void (*RunFrame)(void);		// called once per frame // TODO: add a return value that signals a command to the engine to execute an action
} gameservices_t;

typedef struct
{
	// engine version number
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

	cvar_t *(*CVar_RegisterString)(const char *name, const char *value, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterInt)(const char *name, const int value, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterFloat)(const char *name, const float value, const unsigned long long flags, const char *description);
	cvar_t *(*CVar_RegisterBool)(const char *name, const bool value, const unsigned long long flags, const char *description);

	char *(*CVar_GetString)(cvar_t *cvar);
	int *(*CVar_GetInt)(cvar_t *cvar);
	float *(*CVar_GetFloat)(cvar_t *cvar);
	bool *(*CVar_GetBool)(cvar_t *cvar);
	void (*CVar_SetString)(cvar_t *cvar, const char *value);

	void (*CVar_SetInt)(cvar_t *cvar, const int value);
	void (*CVar_SetFloat)(cvar_t *cvar, const float value);
	void (*CVar_SetBool)(cvar_t *cvar, const bool value);

	// system services
	bool (*Sys_Mkdir)(const char *path);
	char *(*Sys_Strtok)(char *string, const char *delimiter, char **context);
	void (*Sys_Sleep)(unsigned long milliseconds);
	void (*Sys_Localtime)(struct tm *buf, const time_t *timer);

	bool (*Sys_CreateThread)(thread_t *thread, void *(*func)(void *), void *arg);
	void (*Sys_JoinThread)(thread_t thread);

	bool (*Sys_CreateMutex)(mutex_t *mutex);
	void (*Sys_DestroyMutex)(mutex_t *mutex);
	void (*Sys_LockMutex)(mutex_t *mutex);
	void (*Sys_UnlockMutex)(mutex_t *mutex);

	bool (*Sys_CreateCondVar)(condvar_t *condvar);
	void (*Sys_DestroyCondVar)(condvar_t *condvar);
	void (*Sys_WaitCondVar)(condvar_t *condvar, mutex_t *mutex);
	void (*Sys_SignalCondVar)(condvar_t *condvar);

	void *(*Sys_LoadDLL)(const char *dllname);			// returns the DLL handle
	void (*Sys_UnloadDLL)(void *handle);
	void *(*Sys_GetProcAddress)(void *handle, const char *procname);
} mservices_t;
