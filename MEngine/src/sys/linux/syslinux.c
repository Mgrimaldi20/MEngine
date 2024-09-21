#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../common/common.h"

bool Sys_Init(void)
{
	// get the OS version information, only run if the OS version is high enough
	struct utsname osinfo;
	if (uname(&osinfo) == -1)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get OS version information");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "OS: %s %s %s %s", osinfo.sysname, osinfo.nodename, osinfo.release, osinfo.version);

	if (Common_IgnoreOSVer())
		Log_WriteSeq(LOG_WARN, "Ignoring OS version check... "
			"This code has only been tested on Ubuntu 20.04 and version 5.4 "
			"but should work on older OS versions, use with caution");

	else if (strcmp(osinfo.sysname, "Linux") != 0)
	{
		Sys_Error("Requires Linux OS");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());

	CVar_RegisterString("g_gamedll", "DemoGame.so", CVAR_STRING, CVAR_GAME, "The name of the game DLL for Linux systems");

	return(true);
}

void Sys_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down system");
}

void Sys_Error(const char *error, ...)
{
	va_list argptr;
	char buffer[1024] = { 0 };

	va_start(argptr, error);
	vsnprintf(buffer, sizeof(buffer), error, argptr);
	va_end(argptr);

	Log_WriteSeq(LOG_ERROR, buffer);
	printf("Error: %s\n", buffer);

	// TODO: Implement a message box for Linux
}

bool Sys_Mkdir(const char *path)
{
	if (mkdir(path, 0777) != 0)
	{
		if (errno != EEXIST)
			return(false);
	}

	return(true);
}

char *Sys_Strtok(char *string, const char *delimiter, char **context)
{
	return(strtok_r(string, delimiter, context));
}

void Sys_Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

void Sys_Localtime(struct tm *buf, const time_t *timer)
{
	localtime_r(timer, buf);
}

unsigned long long Sys_GetSystemMemory(void)
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);

	return((unsigned long long)pages * (unsigned long long)page_size / (1024 * 1024));
}

bool Sys_ListFiles(const char *directory, const char *filter, sysfiledata_t *filelist, unsigned int numfiles)
{
	DIR *dir = opendir(directory);
	if (!dir)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open directory: %s", directory);
		return(false);
	}

	struct dirent *entry = NULL;
	unsigned int count = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_REG)
		{
			if (filter)
			{
				if (strstr(entry->d_name, filter))
				{
					snprintf(filelist[count].filename, sizeof(filelist[count].filename), "%s/%s", directory, entry->d_name);
					filelist[count].lastwritetime = Sys_FileTimeStamp(filelist[count].filename);
					count++;
				}
			}

			else
			{
				snprintf(filelist[count].filename, sizeof(filelist[count].filename), "%s/%s", directory, entry->d_name);
				filelist[count].lastwritetime = Sys_FileTimeStamp(filelist[count].filename);
				count++;
			}
		}

		if (count >= numfiles)
			break;
	}

	closedir(dir);
	return(true);
}

unsigned int Sys_CountFiles(const char *directory, const char *filter)
{
	DIR *dir = opendir(directory);
	if (!dir)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to open directory: %s", directory);
		return(0);
	}

	struct dirent *entry = NULL;
	unsigned int count = 0;
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_REG)
		{
			if (filter)
			{
				if (strstr(entry->d_name, filter))
					count++;
			}

			else
				count++;
		}
	}

	closedir(dir);
	return(count);
}

time_t Sys_FileTimeStamp(const char *fname)
{
	struct stat filestat;
	if (stat(fname, &filestat) == -1)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get file timestamp: %s", fname);
		return(0);
	}

	return(filestat.st_mtime);
}

unsigned long Sys_GetMaxThreads(void)
{
	return(sysconf(_SC_NPROCESSORS_ONLN));
}

uintptr_t Sys_LoadDLL(const char *dllname)
{
	return((uintptr_t)dlopen(dllname, RTLD_NOW));
}

void Sys_UnloadDLL(uintptr_t handle)
{
	dlclose((void *)handle);
}

void *Sys_GetProcAddress(uintptr_t handle, const char *procname)
{
	return(dlsym((void *)handle, procname));
}
