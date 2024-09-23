#pragma once

#if defined(_WIN64) || defined(WIN64) || defined(__WIN64) || defined(__WIN64__) || defined(__MINGW64__)
#define WIN32_LEAN_AND_MEAN		// stupidly needed for GLU on Windows
#include <Windows.h>
#include <gl/GLU.h>
#elif defined(linux) || defined(__linux) || defined(__linux__) || defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__) || defined(__MACH__)
#include <GL/glu.h>
#else
#error Unknown GL libraries.
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

void *Sys_LoadDLL(const char *dllname);
void Sys_UnloadDLL(void *handle);
void *Sys_GetProcAddress(void *handle, const char *procname);
