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

	CVar_RegisterString("g_gamedll", "DemoGame.dll", CVAR_GAME, "The name of the game DLL for Windows systems");
	CVar_RegisterString("g_demogamedll", "DemoGame.dll", CVAR_GAME | CVAR_READONLY, "The name of the game DLL for Windows systems");

	initialized = true;

	return(true);
}

void Sys_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down system");

	initialized = false;
}

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

void Sys_ProcessCommandLine(cmdline_t *cmdline)
{
	if (!cmdline)
		return;

	wchar_t *saveptr = NULL;
	int count = 0;
	int capacity = 10;

	cmdline->args = malloc(capacity * sizeof(*cmdline->args));
	if (!cmdline->args)
		Sys_Error("Failed to allocate memory for command line arguments");

	wchar_t *token = wcstok(win32state.pcmdline, L" ", &saveptr);
	while (token != NULL)
	{
		if (count >= capacity)
		{
			capacity *= 2;

			char **newargs = realloc(cmdline->args, capacity * sizeof(*cmdline->args));
			if (!newargs)
			{
				free(cmdline->args);
				Sys_Error("Failed to reallocate memory for command line arguments");
			}

			cmdline->args = newargs;
		}

		size_t len = wcslen(token) + 1;

		cmdline->args[count] = malloc(len * sizeof(*cmdline->args[count]));
		if (!cmdline->args[count])
			Sys_Error("Failed to allocate memory for command line argument");

		WideCharToMultiByte(CP_UTF8, 0, token, -1, cmdline->args[count], (int)len, NULL, NULL);
		count++;
		token = wcstok(NULL, L" ", &saveptr);
	}

	cmdline->count = count;
}

bool Sys_Mkdir(const char *path)
{
	if (_mkdir(path) != 0)
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

char *Sys_Strtok(char *string, const char *delimiter, char **context)
{
	return(strtok_s(string, delimiter, context));
}

void Sys_Sleep(unsigned long milliseconds)
{
	Sleep(milliseconds);
}

void Sys_Localtime(struct tm *buf, const time_t *timer)
{
	localtime_s(buf, timer);
}

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

void Sys_CloseDir(void *directory)
{
	FindClose((HANDLE)directory);
}

size_t Sys_GetSystemMemory(void)
{
	MEMORYSTATUSEX meminfo;
	meminfo.dwLength = sizeof(meminfo);
	GlobalMemoryStatusEx(&meminfo);
	return((size_t)(meminfo.ullTotalPhys / 1024 / 1024));
}

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

void Sys_JoinThread(thread_t *thread)
{
	if (thread)
	{
		thrd_join(thread->thread, NULL);
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

	if (mtx_init(&handle->mutex, mtx_plain) != thrd_success)
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
		mtx_destroy(&mutex->mutex);
		mutex->used = false;
	}
}

void Sys_LockMutex(mutex_t *mutex)
{
	mtx_lock(&mutex->mutex);
}

void Sys_UnlockMutex(mutex_t *mutex)
{
#pragma warning(push)
#pragma warning(disable: 26110)
	mtx_unlock(&mutex->mutex);		// complains about not acquiring lock before unlocking, but its just a simple wrapper
#pragma warning(pop)
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

	if (cnd_init(&handle->cond) != thrd_success)
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
		cnd_destroy(&condvar->cond);
		condvar->used = false;
	}
}

void Sys_WaitCondVar(condvar_t *condvar, mutex_t *mutex)
{
	cnd_wait(&condvar->cond, &mutex->mutex);
}

void Sys_SignalCondVar(condvar_t *condvar)
{
	cnd_signal(&condvar->cond);
}

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

void Sys_UnloadDLL(void *handle)
{
	if (handle)
	{
		if (!FreeLibrary(handle))
			WindowsError();
	}
}

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
