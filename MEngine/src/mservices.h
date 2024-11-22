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

typedef struct		// logging service
{
	void (*Write)(logtype_t type, const char *msg, ...);			// write a log message MT
	void (*WriteSeq)(logtype_t type, const char *msg, ...);			// write a log message sequentially
	void (*WriteLargeSeq)(logtype_t type, const char *msg, ...);	// write a log over the max log length, will allocte heap memory
} log_t;

typedef struct		// memory cache allocator and manager
{
	void *(*Alloc)(size_t size);
	void (*Free)(void *ptr);
	void (*Reset)(void);			// reset memory cache, will free all allocated memory and NULL all pointers
	size_t(*GetMemUsed)(void);
	void (*Dump)(void);				// dump memory cache stats to log, mostly for debugging purposes
} memcache_t;

typedef struct		// cvar system
{
	void (*ListAllCVars)(void);		// will write all cvars to the log, mostly for debugging purposes

	cvar_t *(*Find)(const char *name);

	cvar_t *(*RegisterString)(const char *name, const char *value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterInt)(const char *name, const int value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterFloat)(const char *name, const float value, const unsigned long long flags, const char *description);
	cvar_t *(*RegisterBool)(const char *name, const bool value, const unsigned long long flags, const char *description);

	char *(*GetString)(cvar_t *cvar);
	int *(*GetInt)(cvar_t *cvar);
	float *(*GetFloat)(cvar_t *cvar);
	bool *(*GetBool)(cvar_t *cvar);
	void (*SetString)(cvar_t *cvar, const char *value);

	void (*SetInt)(cvar_t *cvar, const int value);
	void (*SetFloat)(cvar_t *cvar, const float value);
	void (*SetBool)(cvar_t *cvar, const bool value);
} cvarsystem_t;

typedef struct		// system services
{
	bool (*Mkdir)(const char *path);
	char *(*Strtok)(char *string, const char *delimiter, char **context);
	void (*Sleep)(unsigned long milliseconds);
	void (*Localtime)(struct tm *buf, const time_t *timer);

	bool (*CreateThread)(thread_t *thread, void *(*func)(void *), void *arg);
	void (*JoinThread)(thread_t *thread);

	bool (*InitMutex)(mutex_t *mutex);
	void (*DestroyMutex)(mutex_t *mutex);
	void (*LockMutex)(mutex_t *mutex);
	void (*UnlockMutex)(mutex_t *mutex);

	bool (*CreateCondVar)(condvar_t *condvar);
	void (*DestroyCondVar)(condvar_t *condvar);
	void (*WaitCondVar)(condvar_t *condvar, mutex_t *mutex);
	void (*SignalCondVar)(condvar_t *condvar);

	void *(*LoadDLL)(const char *dllname);			// returns the DLL handle
	void (*UnloadDLL)(void *handle);
	void *(*GetProcAddress)(void *handle, const char *procname);
} sys_t;

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
	int version;
	log_t *log;
	memcache_t *memcache;
	cvarsystem_t *cvarsystem;
	sys_t *sys;
} mservices_t;
