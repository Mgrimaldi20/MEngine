#pragma once

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

bool Sys_Init(void);
void Sys_Shutdown(void);
void Sys_Error(const char *error, ...);

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

unsigned long Sys_GetMaxThreads(void);

uintptr_t Sys_LoadDLL(const char *dllname);
void Sys_UnloadDLL(uintptr_t handle);
void *Sys_GetProcAddress(uintptr_t handle, const char *procname);
