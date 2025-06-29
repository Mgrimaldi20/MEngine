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
#include <DbgHelp.h>
#include "../../../../EMCrashHandler/src/emstatus.h"

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

static HANDLE emchmapfile;
static HANDLE emchconnevent;
static PVOID vehhandler;

static emstatus_t *emchstatus;

static STARTUPINFO emsi;
static PROCESS_INFORMATION empi;

static cmdline_t cmdline;

static bool initialized;

/*
* Function: VEHCrashHandler
* Executes upon an engine crash, sends the callstack and some signals or messages to the crash handler
* 
*	ei: Pointer to the exception information
* 
* Returns: A signal to the VEH handler to continue searching for the exception
*/
static LONG WINAPI VEHCrashHandler(EXCEPTION_POINTERS *ei)
{
	(EXCEPTION_POINTERS *)ei;

	if (!emchstatus)
		return(EXCEPTION_CONTINUE_EXECUTION);

	HANDLE process = GetCurrentProcess();

	if (!SymInitialize(process, NULL, TRUE))
	{
		snprintf(emchstatus->userdata, EMCH_MAX_USERDATA_SIZE, "Failed to initialize symbol handler");
		return(EXCEPTION_CONTINUE_EXECUTION);
	}

	PVOID stack[EMCH_MAX_FRAMES] = { 0 };
	WORD frames = CaptureStackBackTrace(0, EMCH_MAX_FRAMES, stack, NULL);

	SYMBOL_INFO *symbol = calloc(sizeof(SYMBOL_INFO) + EMCH_MAX_FRAME_SIZE, sizeof(char));
	if (!symbol)
	{
		snprintf(emchstatus->userdata, EMCH_MAX_USERDATA_SIZE, "Failed to allocate memory for symbol, cannot produce backtrace for this itteration");
		return(EXCEPTION_CONTINUE_EXECUTION);
	}

	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
	symbol->MaxNameLen = EMCH_MAX_FRAME_SIZE - 1;

	for (WORD i=0; i<frames; i++)
	{
		if (SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol))
		{
			snprintf(emchstatus->stacktrace[i], EMCH_MAX_FRAME_SIZE, "Frame %u: %s - 0x%llx", i, symbol->Name, (DWORD64)symbol->Address);
			continue;
		}

		snprintf(emchstatus->userdata, EMCH_MAX_USERDATA_SIZE, "Failed to get symbol information for stack frame %u", i);
		snprintf(emchstatus->stacktrace[i], EMCH_MAX_FRAME_SIZE, "Frame %u: Unknown - 0x%llx", i, (DWORD64)stack[i]);
	}

	free(symbol);
	SymCleanup(process);

	emchstatus->status = EMSTATUS_EXIT_CRASH;

	return(EXCEPTION_CONTINUE_EXECUTION);
}

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

	if (!VerifyVersionInfo(&win32state.osver, VER_MAJORVERSION
		| VER_MINORVERSION
		| VER_SERVICEPACKMAJOR
		| VER_SERVICEPACKMINOR, dwlcondmask))
		WindowsError();

	Log_Writef(LOG_INFO, "Windows version: %d.%d.%d.%d",
		win32state.osver.dwMajorVersion,
		win32state.osver.dwMinorVersion,
		win32state.osver.wServicePackMajor,
		win32state.osver.wServicePackMinor
	);

	if (Common_IgnoreOSVer())
		Log_Writef(LOG_WARN, "Ignoring OS version check... "
			"This code has only been tested on Win10 and version 6.2 "
			"but should work on older OS versions, use with caution"
		);

	else if ((win32state.osver.dwMajorVersion < 6)
		|| (win32state.osver.dwMajorVersion == 6 && win32state.osver.dwMinorVersion < 2))
		Sys_Error("Requires Windows 8 or greater");

	Log_Writef(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());
	Log_Writef(LOG_INFO, "System supports max threads: %lu", Sys_GetMaxThreads());

	ULONG_PTR lowlimit = 0;
	ULONG_PTR highlimit = 0;

	GetCurrentThreadStackLimits(&lowlimit, &highlimit);

	uintmax_t stacksize = (uintmax_t)(highlimit - lowlimit);

	Log_Writef(LOG_INFO, "Thread stack limits: Low: %llu, High: %llu", lowlimit, highlimit);
	Log_Writef(LOG_INFO, "Stack size: %juMB", stacksize / (1024 * 1024));

	// create the crash handler shm region and start the process
	emchmapfile = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(emstatus_t),
		L"EMCrashHandlerFileMapping"
	);

	if (!emchmapfile)
	{
		Log_Write(LOG_ERROR, "Failed to create file mapping for crash handler");
		WindowsError();
	}

	if (emchmapfile)	// should always be valid, WindowsError() will call exit()
	{
		emchstatus = MapViewOfFile(
			emchmapfile,
			FILE_MAP_ALL_ACCESS,
			0,
			0,
			sizeof(emstatus_t)
		);

		if (!emchstatus)
		{
			Log_Write(LOG_ERROR, "Failed to map view of file for crash handler");
			WindowsError();
		}
	}

	emchconnevent = CreateEvent(NULL, FALSE, FALSE, L"EMCrashHandlerConnectEvent");
	if (!emchconnevent)
	{
		Log_Write(LOG_ERROR, "Failed to create connect event for crash handler");
		WindowsError();
	}

	vehhandler = AddVectoredExceptionHandler(1, VEHCrashHandler);
	if (!vehhandler)
	{
		Log_Write(LOG_ERROR, "Failed to add VEH handler for crash handler");
		WindowsError();
	}

	ZeroMemory(&emsi, sizeof(emsi));
	emsi.cb = sizeof(emsi);
	ZeroMemory(&empi, sizeof(empi));

	static wchar_t *emchargv[] =	// static so it doesnt disappear
	{
		L"logs",
		NULL
	};

	if (!CreateProcess(
		L"EMCrashHandler.exe",
		emchargv[0],
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&emsi,
		&empi
	))
	{
		Log_Write(LOG_ERROR, "Failed to create Crash Handler process");
		WindowsError();
	}

	if (emchconnevent)
		SetEvent(emchconnevent);

	initialized = true;

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

		if (win32state.errorindicator)
			emchstatus->status = EMSTATUS_EXIT_ERROR;
	}

	if (empi.hProcess)
	{
		CloseHandle(empi.hProcess);
		empi.hProcess = NULL;
	}

	if (empi.hThread)
	{
		CloseHandle(empi.hThread);
		empi.hThread = NULL;
	}

	if (emchstatus)
	{
		UnmapViewOfFile(emchstatus);
		emchstatus = NULL;
	}

	if (emchmapfile)
	{
		CloseHandle(emchmapfile);
		emchmapfile = NULL;
	}

	if (emchconnevent)
	{
		CloseHandle(emchconnevent);
		emchconnevent = NULL;
	}

	if (vehhandler)
	{
		RemoveVectoredExceptionHandler(vehhandler);
		vehhandler = NULL;
	}

	initialized = false;
}

/*
* Function: Sys_Error
* Displays an error message popup window and shuts down the engine, used for fatal errors and exceptions
* 
* 	error: The error message to display, can be a formatted string, max length is LOG_MAX_LEN
*	...: The arguments to the format string, same as printf
*/
void Sys_Error(const char *error, ...)
{
	win32state.errorindicator = true;

	va_list argptr;
	char text[LOG_MAX_LEN] = { 0 };

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	Log_Write(LOG_ERROR, text);

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
* Returns: A pointer to the command line struct, will be NULL if the command line is not set or if it fails to parse, statically allocated struct
*/
cmdline_t *Sys_ParseCommandLine(void)
{
	const int maxcmdlinelen = SYS_MAX_CMD_ARGS * SYS_MAX_CMD_LEN;

	static char cmdlinebuff[SYS_MAX_CMD_LEN * SYS_MAX_CMD_ARGS];

	memset(&cmdline, 0, sizeof(cmdline));	// zero it out to avoid issues if this function is called multiple times

	int len = WideCharToMultiByte(CP_UTF8, 0, win32state.pcmdline, -1, cmdlinebuff, maxcmdlinelen, NULL, NULL);
	if (len == 0)
	{
		Common_Errorf("Failed to convert command line to UTF-8");
		return(NULL);
	}

	if (len >= maxcmdlinelen)
	{
		Common_Errorf("Command line is too long, max length is %d characters", maxcmdlinelen);
		return(NULL);
	}

	int argc = 0;

	char *context = NULL;
	char *token = Sys_Strtok(cmdlinebuff, " ", &context);
	while (token)
	{
		if (argc >= SYS_MAX_CMD_ARGS)
		{
			Common_Errorf("Too many command line arguments, max is %d", SYS_MAX_CMD_ARGS);
			return(NULL);
		}

		snprintf(cmdline.args[argc++], SYS_MAX_CMD_LEN, "%s", token);
		token = Sys_Strtok(NULL, " ", &context);
	}

	cmdline.count = argc;

	if (argc == 0)
	{
		Common_Printf("No command line arguments found");
		return(NULL);
	}

	return(&cmdline);
}

/*
* Function: Sys_IsTTY
* Checks if the console is available, will usually be set if the engine was launched from a console or if the console was created in debug mode
* 
* Returns: A boolean if the console is available or not, will return true if the console is created and is not hidden
*/
bool Sys_IsTTY(void)
{
	return(win32state.conshow);
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
* Creates a new thread, the thread is pulled off a stack of threads of size SYS_MAX_THREADS
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
* Creates a new mutex, the mutex is pulled off a stack of mutexes of size SYS_MAX_MUTEXES
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
* Creates a new condition variable, the condition variable is pulled off a stack of condition variables of size SYS_MAX_CONDVARS
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

/*
* Function: Sys_GetDefDLLName
* Gets the default DLL name for the demo game library for Windows systems
* 
* Returns: The name of the demo game DLL as a statically allocated string
*/
const char *Sys_GetDefDLLName(void)
{
	return("DemoGame.dll");
}
