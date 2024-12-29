#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <pthread.h>
#include <dirent.h>
#include <fnmatch.h>
#include "sys/sys.h"
#include "common/common.h"
#include "posixlocal.h"

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
static condvar_t condvars[SYS_MAX_CONDVARS];

void Sys_Shutdown(void)
{
	if (!posixstate.initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down system");

	posixstate.initialized = false;
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

void Sys_ProcessCommandLine(cmdline_t *cmdline)
{
	if (!cmdline)
		return;

	cmdline->count = posixstate.argc - 1;	// skip the executable name

	cmdline->args = malloc(sizeof(*cmdline->args) * cmdline->count);
	if (!cmdline->args)
	{
		Sys_Error("Failed to allocate memory for command line arguments");
		return;
	}

	for (int i=0; i<cmdline->count; i++)
	{
		size_t len = strlen(posixstate.argv[i + 1]) + 1;

		cmdline->args[i] = malloc(len * sizeof(*cmdline->args[i]));
		if (!cmdline->args[i])
		{
			for (int j=0; j<i; j++)
				free(cmdline->args[j]);

			free(cmdline->args);
			cmdline->args = NULL;

			Sys_Error("Failed to allocate memory for command line argument");
			return;
		}

		snprintf(cmdline->args[i], len, "%s", posixstate.argv[i + 1]);
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

void Sys_Stat(const char *filepath, filedata_t *filedata)
{
	if (!filedata)
		return;

	struct stat st;
	if (stat(filepath, &st) == -1)
	{
		Log_Write(LOG_ERROR, "%s, Failed to make a call to stat(): %s", __func__, filepath);
		return;
	}

	snprintf(filedata->filename, SYS_MAX_PATH, "%s", filepath);
	filedata->filesize = st.st_size;
	filedata->atime = st.st_atime;
	filedata->mtime = st.st_mtime;
	filedata->ctime = st.st_ctime;
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

void *Sys_OpenDir(const char *directory)
{
	DIR *dir = opendir(directory);
	if (!dir)
		return(NULL);

	return(dir);
}

bool Sys_ReadDir(void *directory, char *filename, size_t filenamelen)
{
	struct dirent *entry = readdir((DIR *)directory);
	if (!entry)
		return(false);

	snprintf(filename, filenamelen, "%s", entry->d_name);

	return(true);
}

void Sys_CloseDir(void *directory)
{
	closedir((DIR *)directory);
}

size_t Sys_GetSystemMemory(void)
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);

	return((size_t)pages * (size_t)page_size / (1024 * 1024));
}

unsigned long Sys_GetMaxThreads(void)
{
	return(sysconf(_SC_NPROCESSORS_ONLN));
}

thread_t *Sys_CreateThread(void *(*func)(void *), void *arg)
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
		return(NULL);

	handle->used = true;

	if (pthread_create(&handle->thread, NULL, func, arg) != 0)
	{
		handle->used = false;
		return(NULL);
	}

	return(handle);
}

void Sys_JoinThread(thread_t *thread)
{
	if (thread)
	{
		pthread_join(thread->thread, NULL);
		thread->used = false;
	}
}

mutex_t *Sys_CreateMutex(void)
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
		return(NULL);

	handle->used = true;

	if (pthread_mutex_init(&handle->mutex, NULL) != 0)
	{
		handle->used = false;
		return(NULL);
	}

	return(handle);
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

condvar_t *Sys_CreateCondVar(void)
{
	condvar_t *handle = NULL;
	for (int i=0; !handle && i<SYS_MAX_CONDVARS; i++)
	{
		if (!condvars[i].used)
		{
			handle = &condvars[i];
			break;
		}
	}

	if (!handle)
		return(NULL);

	handle->used = true;

	if (pthread_cond_init(&handle->cond, NULL) != 0)
	{
		handle->used = false;
		return(NULL);
	}

	return(handle);
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
	void *handle = NULL;
	char path[SYS_MAX_PATH] = { 0 };
	char outpath[SYS_MAX_PATH] = { 0 };

	if (getcwd(path, SYS_MAX_PATH))
	{
		int ret = snprintf(outpath, SYS_MAX_PATH, "%s/%s", path, dllname);
		if (ret >= SYS_MAX_PATH)
		{
			Log_WriteSeq(LOG_ERROR, "Path truncated when trying to resolve DLL path");
			return(NULL);
		}

		handle = dlopen(outpath, RTLD_NOW);
	}

	else
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get the current working directory: %s", strerror(errno));
		errno = 0;
		return(NULL);
	}

	if (!handle)
		handle = dlopen(dllname, RTLD_NOW);	// get from LD path if not found in bin dir

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
	if (handle)
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
