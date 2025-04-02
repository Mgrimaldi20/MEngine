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

/*
* Function: SysInitCommon
* Initializes the common system services between MacOS and Linux
* 
* Returns: A boolean if the initialization was successful or not
*/
bool SysInitCommon(void)
{
	if (uname(&posixstate.osinfo) == -1) 	// get the OS version information, only run if the OS version is high enough
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get OS version information");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "OS: %s %s %s %s",
		posixstate.osinfo.sysname,
		posixstate.osinfo.nodename,
		posixstate.osinfo.release,
		posixstate.osinfo.version
	);

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());
	Log_WriteSeq(LOG_INFO, "System supports max threads: %lu", Sys_GetMaxThreads());

	struct rlimit rl;
	if (getrlimit(RLIMIT_STACK, &rl) != 0)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get the stack size");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "Stack size: %juMB", (uintmax_t)(rl.rlim_cur / (1024 * 1024)));

	return(true);
}

/*
* Function: Sys_Shutdown
* Shuts down the system services
*/
void Sys_Shutdown(void)
{
	if (!posixstate.initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down system");

	posixstate.initialized = false;
}

/*
* Function: Sys_Error
* Prints an error message and shuts down the engine, used for fatal and exceptional errors
* 
*	error: The error message to print
*/
void Sys_Error(const char *error, ...)
{
	va_list argptr;
	char buffer[LOG_MAX_LEN] = { 0 };

	va_start(argptr, error);
	vsnprintf(buffer, sizeof(buffer), error, argptr);
	va_end(argptr);

	Log_WriteSeq(LOG_ERROR, buffer);
	Common_Errorf("Error: %s", buffer);

	Common_Shutdown();

	exit(1);
}

/*
* Function: Sys_ParseCommandLine
* Processes the command line arguments and turns the arguments into a command line structure, argc and argv
* 
*	cmdline: The command line structure to fill
*/
void Sys_ParseCommandLine(cmdline_t *cmdline)
{
	if (!cmdline)
		return;

	cmdline->count = posixstate.argc;
	cmdline->args = posixstate.argv;
}

/*
* Function: Sys_Mkdir
* Creates a directory if it does not exist
* 
*	path: The path to the directory to create
* 
* Returns: A boolean if the directory was created or not, also returns true if the directory already exists
*/
bool Sys_Mkdir(const char *path)
{
	if (mkdir(path, 0777) != 0)
	{
		if (errno != EEXIST)
			return(false);
	}

	return(true);
}

/*
* Function: Sys_Stat
* Gets the file status of a file
* 
*	filepath: The full path and name of the file
*	filedata: The filedata_t structure to fill
*/
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

/*
* Function: Sys_Strtok
* A thread safe and portable version of strtok
* 
* 	string: The string to tokenize
*	delimiter: The delimiter to tokenize the string with
*	context: The context to store the current position in the string
* 
* Returns: The next token in the string
*/
char *Sys_Strtok(char *string, const char *delimiter, char **context)
{
	return(strtok_r(string, delimiter, context));
}

/*
* Function: Sys_Strlen
* A portable version of strnlen with a maximum length
* 
*	string: The string to get the length of
*	maxlen: The maximum length of the string to be checked
* 
* Returns: The length of the string or 0 if the string is NULL
*/
size_t Sys_Strlen(const char *string, size_t maxlen)
{
	if (!string)
		return(0);

	return(strnlen(string, maxlen));
}

/*
* Function: Sys_Sleep
* Sleeps the current thread for a specified amount of time
* 
*	milliseconds: The time to sleep in milliseconds
*/
void Sys_Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

/*
* Function: Sys_Localtime
* A thread safe version of localtime
* 
*	buf: The tm structure to fill
*	timer: The time to convert to a tm structure
*/
void Sys_Localtime(struct tm *buf, const time_t *timer)
{
	localtime_r(timer, buf);
}

/*
* Function: Sys_OpenDir
* Opens a directory for reading
* 
*	directory: The directory to open
* 
* Returns: A pointer to the directory handle
*/
void *Sys_OpenDir(const char *directory)
{
	DIR *dir = opendir(directory);
	if (!dir)
		return(NULL);

	return(dir);
}

/*
* Function: Sys_ReadDir
* Reads a directory entry
* 
*	directory: The directory handle
*	filename: The filename to fill
*	filenamelen: The length of the filename buffer
* 
* Returns: A boolean if the read was successful or not
*/
bool Sys_ReadDir(void *directory, char *filename, size_t filenamelen)
{
	struct dirent *entry = readdir((DIR *)directory);
	if (!entry)
		return(false);

	snprintf(filename, filenamelen, "%s", entry->d_name);

	return(true);
}

/*
* Function: Sys_CloseDir
* Closes a directory handle
* 
*	directory: The directory handle to close
*/
void Sys_CloseDir(void *directory)
{
	closedir((DIR *)directory);
}

/*
* Function: Sys_GetSystemMemory
* Gets the total system memory in MB
* 
* Returns: The total system memory in MB
*/
size_t Sys_GetSystemMemory(void)
{
	long pages = sysconf(_SC_PHYS_PAGES);
	long page_size = sysconf(_SC_PAGE_SIZE);

	return((size_t)pages * (size_t)page_size / (1024 * 1024));
}

/*
* Function: Sys_GetMaxThreads
* Gets the maximum number of threads the system can support
* 
* Returns: The maximum number of threads the system can support
*/
unsigned long Sys_GetMaxThreads(void)
{
	return(sysconf(_SC_NPROCESSORS_ONLN));
}

/*
* Function: Sys_CreateThread
* Creates a new thread and runs the specified function
* 
*	func: The function to run in the new thread
*	arg: The arguments to pass to the function
* 
* Returns: A pointer to the thread handle
*/
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

/*
* Function: Sys_JoinThread
* Joins a thread and waits for it to finish
* 
* 	thread: The thread to join
*/
void Sys_JoinThread(thread_t *thread)
{
	if (thread)
	{
		pthread_join(thread->thread, NULL);
		thread->used = false;
	}
}

/*
* Function: Sys_CreateMutex
* Creates a new mutex
* 
* Returns: A pointer to the mutex handle
*/
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

/*
* Function: Sys_DestroyMutex
* Destroys a mutex
* 
*	mutex: The mutex to destroy
*/
void Sys_DestroyMutex(mutex_t *mutex)
{
	if (mutex)
	{
		pthread_mutex_destroy(&mutex->mutex);
		mutex->used = false;
	}
}

/*
* Function: Sys_LockMutex
* Locks a mutex
* 
* 	mutex: The mutex to lock
*/
void Sys_LockMutex(mutex_t *mutex)
{
	pthread_mutex_lock(&mutex->mutex);
}

/*
* Function: Sys_UnlockMutex
* Unlocks a mutex
* 
* 	mutex: The mutex to unlock
*/
void Sys_UnlockMutex(mutex_t *mutex)
{
	pthread_mutex_unlock(&mutex->mutex);
}

/*
* Function: Sys_CreateCondVar
* Creates a new condition variable
* 
* Returns: A pointer to the condition variable handle
*/
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

/*
* Function: Sys_DestroyCondVar
* Destroys a condition variable
* 
*	condvar: The condition variable to destroy
*/
void Sys_DestroyCondVar(condvar_t *condvar)
{
	if (condvar)
	{
		pthread_cond_destroy(&condvar->cond);
		condvar->used = false;
	}
}

/*
* Function: Sys_WaitCondVar
* Waits on a condition variable
* 
*	condvar: The condition variable to wait on
*	mutex: The mutex to lock while waiting
*/
void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex)
{
	pthread_cond_wait(&condvar->cond, &mutex->mutex);
}

/*
* Function: Sys_SignalCondVar
* Signals a condition variable
* 
* 	condvar: The condition variable to signal
*/
void Sys_SignalCondVar(condvar_t *condvar)
{
	pthread_cond_signal(&condvar->cond);
}

/*
* Function: Sys_LoadDLL
* Loads a dynamic link library
* 
* 	dllname: The name of the DLL to load (can be a full path)
* 
* Returns: A pointer to the DLL handle
*/
void *Sys_LoadDLL(const char *dllname)
{
	void *handle = dlopen(dllname, RTLD_NOW);	// get from LD path if not found in bin dir
	if (!handle)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to load DLL: %s", dlerror());
		dlerror();		// clear the error
		return(NULL);
	}

	return(handle);
}

/*
* Function: Sys_UnloadDLL
* Unloads a dynamic link library
* 
*	handle: The handle to the DLL to unload
*/
void Sys_UnloadDLL(void *handle)
{
	if (handle)
		dlclose(handle);
}

/*
* Function: Sys_GetProcAddress
* Gets a procedure address from a DLL
* 
*	handle: The handle to the DLL
*	procname: The name of the procedure to get
* 
* Returns: A pointer to the procedure address
*/
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
