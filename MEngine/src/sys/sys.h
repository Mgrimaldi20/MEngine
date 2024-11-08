#pragma once

#if defined(MENGINE_PLATFORM_WINDOWS)
#define WIN32_LEAN_AND_MEAN		// stupidly needed for GLU on Windows
#include <Windows.h>
#include <gl/GLU.h>
#include <threads.h>
#elif defined(MENGINE_PLATFORM_UNIX)
#include <GL/glu.h>
#include <pthread.h>
#else
#error Unknown GL libraries.
#endif

#define SYS_MAX_CMDLINE_ARGS 32

bool Sys_Init(void);
void Sys_Shutdown(void);
void Sys_Error(const char *error, ...);
void Sys_ProcessCommandLine(char cmdout[SYS_MAX_CMDLINE_ARGS][SYS_MAX_CMDLINE_ARGS]);

bool Sys_Mkdir(const char *path);
char *Sys_Strtok(char *string, const char *delimiter, char **context);
void Sys_Sleep(unsigned long milliseconds);
void Sys_Localtime(struct tm *buf, const time_t *timer);

unsigned long long Sys_GetSystemMemory(void);

typedef struct
{
	time_t lastwritetime;
	char filename[SYS_MAX_PATH];
} sysfiledata_t;

bool Sys_ListFiles(const char *directory, const char *filter, sysfiledata_t *filelist, unsigned int numfiles);
unsigned int Sys_CountFiles(const char *directory, const char *filter);
time_t Sys_FileTimeStamp(const char *fname);

// TODO: make these opaque pointers without the ugly preprocessor hacks
struct thread
{
#if defined(MENGINE_PLATFORM_WINDOWS)
	thrd_t thread;
#elif defined(MENGINE_PLATFORM_UNIX)
	pthread_t thread;
#endif
};

struct mutex
{
#if defined(MENGINE_PLATFORM_WINDOWS)
	mtx_t mutex;
#elif defined(MENGINE_PLATFORM_UNIX)
	pthread_mutex_t mutex;
#endif
};

struct condvar
{
#if defined(MENGINE_PLATFORM_WINDOWS)
	cnd_t cond;
#elif defined(MENGINE_PLATFORM_UNIX)
	pthread_cond_t cond;
#endif
};

unsigned long Sys_GetMaxThreads(void);

bool Sys_CreateThread(thread_t *thread, void *(*func)(void *), void *arg);
void Sys_JoinThread(thread_t thread);

bool Sys_CreateMutex(mutex_t *mutex);
void Sys_DestroyMutex(mutex_t *mutex);
void Sys_LockMutex(mutex_t *mutex);
void Sys_UnlockMutex(mutex_t *mutex);

bool Sys_CreateCondVar(condvar_t *condvar);
void Sys_DestroyCondVar(condvar_t *condvar);
void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex);
void Sys_SignalCondVar(condvar_t *condvar);

void *Sys_LoadDLL(const char *dllname);
void Sys_UnloadDLL(void *handle);
void *Sys_GetProcAddress(void *handle, const char *procname);
