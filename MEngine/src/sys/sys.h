#pragma once

#include "common/common.h"

bool Sys_Init(void);
void Sys_Shutdown(void);
void Sys_Error(const char *error, ...);

#define SYS_MAX_CMD_ARGS 64
#define SYS_MAX_CMD_LEN 512

typedef struct
{
	char args[SYS_MAX_CMD_ARGS][SYS_MAX_CMD_LEN];
	int count;
} cmdline_t;

cmdline_t *Sys_ParseCommandLine(void);

bool Sys_IsTTY(void);
bool Sys_Mkdir(const char *path);
void Sys_Stat(const char *filepath, filedata_t *filedata);
char *Sys_Strtok(char *string, const char *delimiter, char **context);
size_t Sys_Strlen(const char *string, size_t maxlen);
void Sys_Sleep(unsigned long milliseconds);
void Sys_Localtime(struct tm *buf, const time_t *timer);

void *Sys_OpenDir(const char *directory);
bool Sys_ReadDir(void *directory, char *filename, size_t filenamelen);
void Sys_CloseDir(void *directory);

size_t Sys_GetSystemMemory(void);

#define SYS_MAX_THREADS 64
#define SYS_MAX_MUTEXES 64
#define SYS_MAX_CONDVARS 64

unsigned long Sys_GetMaxThreads(void);

thread_t *Sys_CreateThread(void *(*func)(void *), void *arg);
void Sys_JoinThread(thread_t *thread);

mutex_t *Sys_CreateMutex(void);
void Sys_DestroyMutex(mutex_t *mutex);
void Sys_LockMutex(mutex_t *mutex);
void Sys_UnlockMutex(mutex_t *mutex);

condvar_t *Sys_CreateCondVar(void);
void Sys_DestroyCondVar(condvar_t *condvar);
void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex);
void Sys_SignalCondVar(condvar_t *condvar);

void *Sys_LoadDLL(const char *dllname);
void Sys_UnloadDLL(void *handle);
void *Sys_GetProcAddress(void *handle, const char *procname);
