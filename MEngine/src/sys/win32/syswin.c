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

bool Sys_Init(void)
{
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
	CVar_RegisterString("g_demogamedll", "DemoGame.dll", CVAR_GAME, "The name of the game DLL for Windows systems");

	return(true);
}

void Sys_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down system");
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

void Sys_ProcessCommandLine(char cmdout[SYS_MAX_CMDLINE_ARGS][SYS_MAX_CMDLINE_ARGS])
{
	wchar_t *saveptr = NULL;
	int i = 0;

	wchar_t *token = wcstok(win32state.pcmdline, L" -", &saveptr);
	while (token != NULL)
	{
		WideCharToMultiByte(CP_UTF8, 0, token, -1, cmdout[i], SYS_MAX_CMDLINE_ARGS, NULL, NULL);
		token = wcstok(NULL, L" -", &saveptr);
		i++;
	}
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

filedata_t Sys_Stat(const char *filepath)
{
	struct _stat st;
	if (_stat(filepath, &st) == -1)
	{
		Log_Write(LOG_ERROR, "%s, Failed to make a call to stat(): %s", __func__, filepath);
		return((filedata_t){ 0 });
	}

	filedata_t data;
	snprintf(data.filename, SYS_MAX_PATH, "%s", filepath);
	data.filesize = st.st_size;
	data.lastwritetime = st.st_mtime;

	// TODO: add more file data here
	return(data);
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

unsigned long long Sys_GetSystemMemory(void)
{
	MEMORYSTATUSEX meminfo;
	meminfo.dwLength = sizeof(meminfo);
	GlobalMemoryStatusEx(&meminfo);
	return((meminfo.ullTotalPhys / 1024 / 1024));
}

bool Sys_ListFiles(const char *directory, const char *filter, sysfiledata_t *filelist, unsigned int numfiles)
{
	if (!filelist)
		return(false);

	wchar_t wdirectory[SYS_MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, directory, -1, wdirectory, SYS_MAX_PATH);

	wchar_t searchpath[SYS_MAX_PATH] = { 0 };
	if (filter)
	{
		wchar_t wfilter[SYS_MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, filter, -1, wfilter, SYS_MAX_PATH);
		swprintf(searchpath, sizeof(searchpath) / sizeof(*searchpath), L"%s\\%s", wdirectory, wfilter);
	}

	else
		swprintf(searchpath, sizeof(searchpath) / sizeof(*searchpath), L"%s\\*", wdirectory);

	WIN32_FIND_DATA findfiledata;
	HANDLE hfind = FindFirstFile(searchpath, &findfiledata);

	if (hfind == INVALID_HANDLE_VALUE)
		return(false);

	unsigned int count = 0;

	do
	{
		if (!(findfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{
			char temp[SYS_MAX_PATH] = { 0 };
			WideCharToMultiByte(CP_UTF8, 0, findfiledata.cFileName, -1, temp, SYS_MAX_PATH, NULL, NULL);

			snprintf(filelist[count].filename, SYS_MAX_PATH, "%s/%s", directory, temp);
			filelist[count].lastwritetime = Sys_FileTimeStamp(filelist[count].filename);
			count++;
		}
	} while (FindNextFile(hfind, &findfiledata) != 0 && (numfiles == 0 || count < numfiles));	// files obtained unsorted on Win32

	FindClose(hfind);
	return(true);
}

unsigned int Sys_CountFiles(const char *directory, const char *filter)
{
	int filecount = 0;

	wchar_t wdirectory[SYS_MAX_PATH] = { 0 };
	MultiByteToWideChar(CP_UTF8, 0, directory, -1, wdirectory, SYS_MAX_PATH);

	wchar_t searchpath[SYS_MAX_PATH] = { 0 };
	if (filter != NULL)
	{
		wchar_t wfilter[SYS_MAX_PATH] = { 0 };
		MultiByteToWideChar(CP_UTF8, 0, filter, -1, wfilter, SYS_MAX_PATH);
		swprintf(searchpath, sizeof(searchpath) / sizeof(*searchpath), L"%s\\%s", wdirectory, wfilter);
	}

	else
		swprintf(searchpath, sizeof(searchpath) / sizeof(*searchpath), L"%s\\*", wdirectory);

	WIN32_FIND_DATA findfiledata;
	HANDLE hfind = FindFirstFile(searchpath, &findfiledata);
	if (hfind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(findfiledata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				filecount++;
		} while (FindNextFile(hfind, &findfiledata) != 0);

		FindClose(hfind);
	}

	return(filecount);
}

time_t Sys_FileTimeStamp(const char *fname)
{
	struct _stat st;
	_stat(fname, &st);
	return(st.st_mtime);
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
	wchar_t wdllname[MAX_PATH] = { 0 };
	if (!MultiByteToWideChar(CP_UTF8, 0, dllname, -1, wdllname, MAX_PATH))
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
