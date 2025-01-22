#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <direct.h>
#include <process.h>
#include <sys/stat.h>
#include <threads.h>
#include "sys/sys.h"
#include "common/common.h"
#include "winlocal.h"

struct thread
{
	thrd_t thread;
	bool used;
};

struct mutex
{
	mtx_t mutex;
	bool used;
};

struct condvar
{
	cnd_t cond;
	bool used;
};

static thread_t threads[SYS_MAX_THREADS];
static mutex_t mutexes[SYS_MAX_MUTEXES];
static condvar_t condvars[SYS_MAX_CONDVARS];

static bool initialized;

/*
* Function: Sys_Init
* Initializes the system services and gets the OS system information for Windows systems
* 
* Returns: A boolean if the initialization was successful or not
*/
bool Sys_Init(void)
{
	if (initialized)
		return(true);

	DWORDLONG dwlcondmask = 0;

	ZeroMemory(&win32state.osver, sizeof(OSVERSIONINFOEX));
	win32state.osver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	win32state.osver.dwMajorVersion = HIBYTE(_WIN32_WINNT_WIN8);
	win32state.osver.dwMinorVersion = LOBYTE(_WIN32_WINNT_WIN8);
	win32state.osver.wServicePackMajor = 0;
	win32state.osver.wServicePackMinor = 0;

	VER_SET_CONDITION(dwlcondmask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlcondmask, VER_MINORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlcondmask, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlcondmask, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

	if (!VerifyVersionInfo(&win32state.osver, VER_MAJORVERSION |
		VER_MINORVERSION |
		VER_SERVICEPACKMAJOR |
		VER_SERVICEPACKMINOR, dwlcondmask))
		WindowsError();

	Log_WriteSeq(LOG_INFO, "Windows version: %d.%d.%d.%d",
		win32state.osver.dwMajorVersion,
		win32state.osver.dwMinorVersion,
		win32state.osver.wServicePackMajor,
		win32state.osver.wServicePackMinor
	);

	if (Common_IgnoreOSVer())
		Log_WriteSeq(LOG_WARN, "Ignoring OS version check... "
			"This code has only been tested on Win10 and version 6.2 "
			"but should work on older OS versions, use with caution");

	else if ((win32state.osver.dwMajorVersion < 6) ||
		(win32state.osver.dwMajorVersion == 6 && win32state.osver.dwMinorVersion < 2))
	{
		Sys_Error("Requires Windows 8 or greater");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());
	Log_WriteSeq(LOG_INFO, "System supports max threads: %lu", Sys_GetMaxThreads());

	ULONG_PTR lowlimit = 0;
	ULONG_PTR highlimit = 0;

	GetCurrentThreadStackLimits(&lowlimit, &highlimit);

	uintmax_t stacksize = (uintmax_t)(highlimit - lowlimit);

	Log_WriteSeq(LOG_INFO, "Thread stack limits: Low: %llu, High: %llu", lowlimit, highlimit);
	Log_WriteSeq(LOG_INFO, "Stack size: %juMB", stacksize / (1024 * 1024));

	CVar_RegisterString("g_gamedll", "DemoGame.dll", CVAR_GAME, "The name of the game DLL for Windows systems");
	CVar_RegisterString("g_demogamedll", "DemoGame.dll", CVAR_GAME | CVAR_READONLY, "The name of the game DLL for Windows systems");

	initialized = true;

	return(true);
}

/*
* Function: Sys_Shutdown
* Shuts down the system services
*/
void Sys_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down system");

	initialized = false;
}

/*
* Function: Sys_Error
* Displays an error message popup window and shuts down the engine, used for fatal errors and exceptions
* 
* 	error: The error message to display
*/
void Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[LOG_MAX_LEN] = { 0 };

	va_start(argptr, error);
	vsnprintf(text, LOG_MAX_LEN, error, argptr);
	va_end(argptr);

	Log_WriteSeq(LOG_ERROR, text);

	wchar_t wtext[LOG_MAX_LEN] = { 0 };
	if (!MultiByteToWideChar(CP_UTF8, 0, text, (int)sizeof(text), wtext, LOG_MAX_LEN))
		return;

	MessageBox(NULL, wtext, L"Error", MB_OK);

	Common_Shutdown();
	ShutdownConsole();

	exit(1);
}

/*
* Function: Sys_ParseCommandLine
* Processes the command line arguments and stores them in a command line struct, turns the wide char command line into argc and argv
* 
*	cmdline: The command line struct to store the arguments in
*/
void Sys_ParseCommandLine(cmdline_t *cmdline)
{
	if (!cmdline)
		return;

	int len = WideCharToMultiByte(CP_UTF8, 0, win32state.pcmdline, -1, win32state.cmdline, MAX_CMDLINE, NULL, NULL);
	if (len == 0)
		Sys_Error("Failed to convert command line to UTF-8");

	int argc = 0;
	char *argv[MAX_CMDLINE] = { 0 };

	char *context = NULL;
	char *token = Sys_Strtok(win32state.cmdline, " ", &context);
	while (token)
	{
		if (argc >= MAX_CMDLINE)
			Sys_Error("Too many command line arguments");

		argv[argc++] = token;
		token = Sys_Strtok(NULL, " ", &context);
	}

	cmdline->count = argc;
	cmdline->args = argv;
}

/*
* Function: Sys_Mkdir
* Creates a directory
* 
*	path: The path of the directory to create
* 
* Returns: A boolean if the directory was created or not, also returns true if the directory already exists
*/
bool Sys_Mkdir(const char *path)
{
	if (_mkdir(path) != 0)
	{
		if (errno != EEXIST)
			return(false);
	}

	return(true);
}

/*
* Function: Sys_Stat
* Gets the file information for a file
* 
*	filepath: The path of the file to get the information for
*	filedata: The filedata struct to store the file information in
*/
void Sys_Stat(const char *filepath, filedata_t *filedata)
{
	if (!filedata)
		return;

	struct _stat st;
	if (_stat(filepath, &st) == -1)
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
* Portable version of strtok_s and thread safe
* 
*	string: The string to tokenize
*	delimiter: The delimiter to tokenize the string with
*	context: The context to store the current position in the string
* 
* Returns: A pointer to the next token in the string
*/
char *Sys_Strtok(char *string, const char *delimiter, char **context)
{
	return(strtok_s(string, delimiter, context));
}

/*
* Function: Sys_Strlen
* Portable version of strnlen_s
* 
*	string: The string to get the length of
*	maxlen: The maximum length of the string
* 
* Returns: The length of the string
*/
size_t Sys_Strlen(const char *string, size_t maxlen)
{
	return(strnlen_s(string, maxlen));
}

/*
* Function: Sys_Sleep
* Sleeps the current thread for a specified amount of time
* 
*	milliseconds: The amount of time to sleep in milliseconds
*/
void Sys_Sleep(unsigned long milliseconds)
{
	Sleep(milliseconds);
}

/*
* Function: Sys_Localtime
* Portable version of localtime_s, thread safe
* 
*	buf: The buffer to store the time information in
*	timer: The time to get the local time from
*/
void Sys_Localtime(struct tm *buf, const time_t *timer)
{
	localtime_s(buf, timer);
}

/*
* Function: Sys_OpenDir
* Opens a directory for reading
* 
*	directory: The directory path to open
* 
* Returns: A pointer to the directory handle
*/
void *Sys_OpenDir(const char *directory)
{
	WIN32_FIND_DATA findfiledata = { 0 };

	wchar_t wdirectory[SYS_MAX_PATH] = { 0 };
	if (!MultiByteToWideChar(CP_UTF8, 0, directory, -1, wdirectory, SYS_MAX_PATH))
		return(NULL);

	wchar_t wsearchpath[SYS_MAX_PATH] = { 0 };
	swprintf(wsearchpath, SYS_MAX_PATH, L"%s\\*", wdirectory);

	HANDLE handle = FindFirstFile(wsearchpath, &findfiledata);
	if (handle == INVALID_HANDLE_VALUE)
		return(NULL);

	return(handle);
}

/*
* Function: Sys_ReadDir
* Reads a directory and gets the next file in the directory
* 
*	directory: The directory handle to read from
*	filename: The buffer to store the filename in
*	filenamelen: The length of the filename buffer
* 
* Returns: A boolean if the file was read or not
*/
bool Sys_ReadDir(void *directory, char *filename, size_t filenamelen)
{
	HANDLE handle = (HANDLE)directory;
	WIN32_FIND_DATA findfiledata = { 0 };

	if (!FindNextFile(handle, &findfiledata))
		return(false);

	if (!WideCharToMultiByte(CP_UTF8, 0, findfiledata.cFileName, -1, filename, (int)filenamelen, NULL, NULL))	// saves the filename as a UTF-8 string
		return(false);

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
	FindClose((HANDLE)directory);
}

/*
* Function: Sys_GetSystemMemory
* Gets the total system memory in MB
* 
* Returns: The total system memory in MB
*/
size_t Sys_GetSystemMemory(void)
{
	MEMORYSTATUSEX meminfo;
	meminfo.dwLength = sizeof(meminfo);
	GlobalMemoryStatusEx(&meminfo);
	return((size_t)(meminfo.ullTotalPhys / 1024 / 1024));
}

/*
* Function: Sys_GetMaxThreads
* Gets the maximum number of threads the system supports
* 
* Returns: The maximum number of threads the system supports
*/
unsigned long Sys_GetMaxThreads(void)
{
	static unsigned long maxthreads;

	if (maxthreads == 0)
	{
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		maxthreads = sysinfo.dwNumberOfProcessors;
	}

	return(maxthreads);
}

/*
* Function: Sys_CreateThread
* Creates a new thread
* 
*	func: The function to run in the new thread
*	arg: The arguments to pass to the function
* 
* Returns: A pointer to the new thread handle
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

	if (thrd_create(&handle->thread, (thrd_start_t)func, arg) != thrd_success)
	{
		handle->used = false;
		return(NULL);
	}

	return(handle);
}

/*
* Function: Sys_JoinThread
* Joins a thread
* 
*	thread: The thread to join
*/
void Sys_JoinThread(thread_t *thread)
{
	if (thread)
	{
		thrd_join(thread->thread, NULL);
		thread->used = false;
	}
}

/*
* Function: Sys_CreateMutex
* Creates a new mutex
* 
* Returns: A pointer to the new mutex handle
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

	if (mtx_init(&handle->mutex, mtx_plain) != thrd_success)
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
		mtx_destroy(&mutex->mutex);
		mutex->used = false;
	}
}

/*
* Function: Sys_LockMutex
* Locks a mutex
* 
*	mutex: The mutex to lock
*/
void Sys_LockMutex(mutex_t *mutex)
{
	mtx_lock(&mutex->mutex);
}

/*
* Function: Sys_UnlockMutex
* Unlocks a mutex, ignores some warnings about not acquiring the lock before unlocking, this is expected behavior
* 
*	mutex: The mutex to unlock
*/
void Sys_UnlockMutex(mutex_t *mutex)
{
#pragma warning(push)
#pragma warning(disable: 26110)
	mtx_unlock(&mutex->mutex);		// complains about not acquiring lock before unlocking, but its just a simple wrapper
#pragma warning(pop)
}

/*
* Function: Sys_CreateCondVar
* Creates a new condition variable
* 
*	Returns: A pointer to the new condition variable handle
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

	if (cnd_init(&handle->cond) != thrd_success)
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
		cnd_destroy(&condvar->cond);
		condvar->used = false;
	}
}

/*
* Function: Sys_WaitCondVar
* Waits on a condition variable
* 
*	condvar: The condition variable to wait on
*	mutex: The mutex to lock
*/
void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex)
{
	cnd_wait(&condvar->cond, &mutex->mutex);
}

/*
* Function: Sys_SignalCondVar
* Signals a condition variable
* 
*	condvar: The condition variable to signal
*/
void Sys_SignalCondVar(condvar_t *condvar)
{
	cnd_signal(&condvar->cond);
}

/*
* Function: Sys_LoadDLL
* Loads a DLL
* 
*	dllname: The name of the DLL to load
* 
* Returns: A pointer to the DLL handle
*/
void *Sys_LoadDLL(const char *dllname)
{
	wchar_t wdllname[SYS_MAX_PATH] = { 0 };
	if (!MultiByteToWideChar(CP_UTF8, 0, dllname, -1, wdllname, SYS_MAX_PATH))
		return(NULL);

	HMODULE libhandle = LoadLibrary(wdllname);
	if (!libhandle)
		WindowsError();

	return(libhandle);
}

/*
* Function: Sys_UnloadDLL
* Unloads a DLL
* 
*	handle: The DLL handle to unload
*/
void Sys_UnloadDLL(void *handle)
{
	if (handle)
	{
		if (!FreeLibrary(handle))
			WindowsError();
	}
}

/*
* Function: Sys_GetProcAddress
* Gets a procedure address from a DLL that has been loaded
* 
*	handle: The DLL handle to get the procedure address from
*	procname: The name of the procedure to get the address of
* 
* Returns: A pointer to the procedure address
*/
void *Sys_GetProcAddress(void *handle, const char *procname)
{
#pragma warning(push)
#pragma warning(disable: 4152)
	FARPROC proc = GetProcAddress(handle, procname);	// non standard cast but its a sys function so thats okay
	
	if (!proc)
		WindowsError();

	return(proc);
#pragma warning(pop)
}
