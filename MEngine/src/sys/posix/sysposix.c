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
#include <fcntl.h>
#include <sys/mman.h>
#include "sys/sys.h"
#include "common/common.h"
#include "posixlocal.h"
#include "../../../../EMCrashHandler/src/emstatus.h"

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

static int emchfd;
static emstatus_t *emchstatus;

static pid_t emchpid;

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
		Log_Write(LOG_ERROR, "Failed to get OS version information");
		return(false);
	}

	Log_Writef(LOG_INFO, "OS: %s %s %s %s",
		posixstate.osinfo.sysname,
		posixstate.osinfo.nodename,
		posixstate.osinfo.release,
		posixstate.osinfo.version
	);

	Log_Writef(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());
	Log_Writef(LOG_INFO, "System supports max threads: %lu", Sys_GetMaxThreads());

	struct rlimit rl;
	if (getrlimit(RLIMIT_STACK, &rl) == 0)
		Log_Writef(LOG_INFO, "Stack size: %juMB", (uintmax_t)(rl.rlim_cur / (1024 * 1024)));

	// create the crash handler shm region and start the process
	emchfd = shm_open("/EMCrashHandlerFileMapping", O_CREAT | O_EXCL | O_RDWR, 0600);
	if (emchfd == -1)
		Sys_Error("Failed to create shared memory file mapping: %s", strerror(errno));

	if (ftruncate(emchfd, sizeof(emstatus_t)) == -1)
		Sys_Error("Failed to set the size of the shared memory file mapping: %s", strerror(errno));

	emchstatus = mmap(NULL, sizeof(emstatus_t), PROT_READ | PROT_WRITE, MAP_SHARED, emchfd, 0);
	if (emchstatus == MAP_FAILED)
		Sys_Error("Failed to map the shared memory file mapping: %s", strerror(errno));

	switch (fork())
	{
		case -1:
			Sys_Error("Failed to fork the process to start the crash handler: %s", strerror(errno));	// calls exit(), no need to break

		case 0:		// child process
			if (execl("EMCrashHandler", "logs", NULL) == -1)
				Sys_Error("Failed to start the crash handler process: %s", strerror(errno));

		default:	// parent process, fallthrough
			emchpid = getpid();
			break;
	}

	if (shm_unlink("/EMCrashHandlerFileMapping") == -1)
		Sys_Error("Failed to unlink the shared memory file mapping: %s", strerror(errno));

	return(true);
}

/*
* Function: Sys_Shutdown
* Shuts down the system services
*/
void Sys_Shutdown(void)
{
	Log_Write(LOG_INFO, "Shutting down system");

	if (emchstatus)
	{
		emchstatus->status = EMSTATUS_EXIT_OK;

		if (!posixstate.errorindicator)
			emchstatus->status = EMSTATUS_EXIT_ERROR;
	}

	if (emchstatus)
	{
		munmap(emchstatus, sizeof(emstatus_t));
		emchstatus = NULL;
	}

	if (emchfd != -1)
	{
		close(emchfd);
		emchfd = -1;
	}

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

	Common_Errorf("Error: %s", buffer);

	Common_Shutdown();

	exit(1);
}

/*
* Function: Sys_ParseCommandLine
* Processes the command line arguments and stores them in a command line struct, turns the wide char command line into argc and argv
* 
* Returns: A pointer to the command line struct, will be NULL if the command line is not set or if it fails to parse, statically allocated struct
*/
cmdline_t *Sys_ParseCommandLine(void)
{
	static cmdline_t cmdline;

	memset(&cmdline, 0, sizeof(cmdline));	// zero it out to avoid issues if this function is called multiple times

	if (posixstate.argc == 0)
	{
		Common_Printf("No command line arguments found");
		return(NULL);
	}

	if (posixstate.argc > SYS_MAX_CMD_ARGS)
	{
		Common_Errorf("Too many command line arguments, maximum is %d", SYS_MAX_CMD_ARGS);
		return(NULL);
	}

	cmdline.count = posixstate.argc;

	for (int i=0; i<cmdline.count; i++)
		snprintf(cmdline.args[i], SYS_MAX_CMD_LEN, "%s", posixstate.argv[i]);

	return(&cmdline);
}

/*
* Function: Sys_IsTTY
* Checks if the console is available, will usually be set if the engine was launched from a console
* 
* Returns: A boolean if the console is available or not
*/
bool Sys_IsTTY(void)
{
	errno = 0;
	bool res = isatty(fileno(stdin));

	if (errno == EBADF)
		Common_Errorf("%s: FD is invalid: %s", __func__, strerror(errno));

	if (errno == ENOTTY)
		Common_Warnf("%s: No TTY, will not print output, terminal might be required for error output: %s",
			__func__,
			strerror(errno)
		);

	if (errno == EINVAL)
		Common_Warnf("%s: This is not POSIX compliant! But I am nice enough to give you the warning anyway: %s",
			__func__,
			strerror(errno)
		);

	return(res);
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
		Log_Writef(LOG_ERROR, "%s, Failed to make a call to stat(): %s", __func__, filepath);
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
	struct dirent * const entry = readdir((DIR *)directory);
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
	void *handle = dlopen(dllname, RTLD_NOW);
	if (!handle)
	{
		Log_Writef(LOG_ERROR, "Failed to load DLL: %s", dlerror());
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
		Log_Writef(LOG_ERROR, "Failed to get procedure address: %s", dlerror());
		dlerror();		// clear the error
		return(NULL);
	}

	return(proc);
}
