#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../../common/common.h"
#include "linuxlocal.h"

struct thread
{
	pthread_t thread;
	bool used;
};

struct mutex
{
	pthread_mutex_t mutex;
	bool used;
};

struct condvar
{
	pthread_cond_t cond;
	bool used;
};

static thread_t threads[SYS_MAX_THREADS];
static mutex_t mutexes[SYS_MAX_MUTEXES];
static condvar_t condvars[SYS_MAX_COND_VARS];

bool Sys_Init(void)
{
	// get the OS version information, only run if the OS version is high enough
	if (uname(&linuxstate.osinfo) == -1)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get OS version information");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "OS: %s %s %s %s",
		linuxstate.osinfo.sysname,
		linuxstate.osinfo.nodename,
		linuxstate.osinfo.release,
		linuxstate.osinfo.version
	);

	if (Common_IgnoreOSVer())
		Log_WriteSeq(LOG_WARN, "Ignoring OS version check... "
			"This code has only been tested on Windows systems so far "
			"but the common and shared code should work the same, use with caution, "
			"some of the system frameworks might not work as expected");

	else if (strcmp(linuxstate.osinfo.sysname, "Linux") != 0)
	{
		Sys_Error("Requires Linux OS");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());

	CVar_RegisterString("g_gamedll", "DemoGame.so", CVAR_GAME, "The name of the game DLL for Linux systems");
	CVar_RegisterString("g_demogamedll", "DemoGame.so", CVAR_GAME, "The name of the demo game DLL for Linux systems");

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
	fprintf(stderr, "Error: %s\n", buffer);

	Common_Shutdown();

	exit(1);
}

void Sys_ProcessCommandLine(char cmdout[SYS_MAX_CMDLINE_ARGS][SYS_MAX_CMDLINE_ARGS])
{
	char *saveptr = NULL;
	int i = 0;

	for (int j=1; j<linuxstate.argc&&i<SYS_MAX_CMDLINE_ARGS; j++)
	{
		char *token = Sys_Strtok(linuxstate.argv[j], " -", &saveptr);
		while (token != NULL)
		{
			snprintf(cmdout[i], SYS_MAX_CMDLINE_ARGS, "%s", token);
			token = Sys_Strtok(NULL, " -", &saveptr);
			i++;
		}
	}
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

bool Sys_CreateThread(thread_t *thread, void *(*func)(void *), void *arg)
{
	thread_t *handle = NULL;
	for (int i=0; !handle && i<SYS_MAX_THREADS; i++)
	{
		if (!threads[i].used)
		{
			handle = &threads[i];
			break;
		}
	}

	if (!handle)
		return(false);

	handle->used = true;

	if (pthread_create(&handle->thread, NULL, func, arg) != 0)
	{
		handle->used = false;
		return(false);
	}

	thread = handle;
	return(true);
}

void Sys_JoinThread(thread_t *thread)
{
	if (thread)
	{
		pthread_join(thread->thread, NULL);
		thread->used = false;
	}
}

bool Sys_CreateMutex(mutex_t *mutex)
{
	mutex_t *handle = NULL;
	for (int i=0; !handle && i<SYS_MAX_MUTEXES; i++)
	{
		if (!mutexes[i].used)
		{
			handle = &mutexes[i];
			break;
		}
	}

	if (!handle)
		return(false);

	handle->used = true;

	if (pthread_mutex_init(&handle->mutex, NULL) != 0)
	{
		handle->used = false;
		return(false);
	}

	mutex = handle;
	return(true);
}

void Sys_DestroyMutex(mutex_t *mutex)
{
	if (mutex)
	{
		pthread_mutex_destroy(&mutex->mutex);
		mutex->used = false;
	}
}

void Sys_LockMutex(mutex_t *mutex)
{
	pthread_mutex_lock(&mutex->mutex);
}

void Sys_UnlockMutex(mutex_t *mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
}

bool Sys_CreateCondVar(condvar_t *condvar)
{
	condvar_t *handle = NULL;
	for (int i=0; !handle && i<SYS_MAX_COND_VARS; i++)
	{
		if (!condvars[i].used)
		{
			handle = &condvars[i];
			break;
		}
	}

	if (!handle)
		return(false);

	handle->used = true;

	if (pthread_cond_init(&handle->cond, NULL) != 0)
	{
		handle->used = false;
		return(false);
	}

	condvar = handle;
	return(true);
}

void Sys_DestroyCondVar(condvar_t *condvar)
{
	if (condvar)
	{
		pthread_cond_destroy(&condvar->cond);
		condvar->used = false;
	}
}

void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex)
{
	pthread_cond_wait(&condvar->cond, &mutex->mutex);
}

void Sys_SignalCondVar(condvar_t *condvar)
{
	pthread_cond_signal(&condvar->cond);
}

void *Sys_LoadDLL(const char *dllname)
{
	void *handle = dlopen(dllname, RTLD_NOW);
	if (!handle)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to load DLL: %s", dlerror());
		dlerror();		// clear the error
		return(NULL);
	}

	return(handle);
}

void Sys_UnloadDLL(void *handle)
{
	dlclose(handle);
}

void *Sys_GetProcAddress(void *handle, const char *procname)
{
	void *proc = dlsym(handle, procname);
	if (!proc)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get procedure address: %s", dlerror());
		dlerror();		// clear the error
		return(NULL);
	}

	return(proc);
}
