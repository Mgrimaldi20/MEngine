#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <shellapi.h>
#include <stdio.h>
#include "../emstatus.h"

#pragma warning(disable: 28251)		// disables windows annotations warning

static void WindowsError(void)
{
	LPVOID wlpmsgbuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&wlpmsgbuf,
		0,
		NULL
	);

	MessageBox(NULL, wlpmsgbuf, L"EMCrashHandler: GetLastError", MB_OK | MB_ICONINFORMATION);
	LocalFree(wlpmsgbuf);
}

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, PWSTR pcmdline, int ncmdshow)
{
	(HINSTANCE)hinst;
	(HINSTANCE)hprevinst;
	(int)ncmdshow;

	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(pcmdline, &argc);	// quirky function that will not return the exe name in the string if pcmdline is not NULL
	if (!argv)
	{
		WindowsError();
		return(1);
	}

	const wchar_t *logfilename = L"crashstatus.log";
	wchar_t logfullpath[MAX_PATH] = { 0 };

	_snwprintf(logfullpath, MAX_PATH - 1, L"%s", logfilename);

	if (pcmdline)
		_snwprintf(logfullpath, MAX_PATH - 1, L"%s\\%s", argv[0], logfilename);

	LocalFree(argv);	// CommandLineToArgvW allocates memory for argv

	logfullpath[MAX_PATH - 1] = L'\0';
	FILE *logfile = _wfopen(logfullpath, L"w+");
	if (!logfile)
	{
		wchar_t errormsg[1024] = { 0 };
		_snwprintf(errormsg, 1023, L"Cannot open log file: %s, directory might not exist", logfullpath);
		errormsg[1023] = L'\0';

		MessageBox(NULL, errormsg, L"EMCrashHandler: Cannot Open Log File", MB_OK | MB_ICONINFORMATION);
		return(1);
	}

	HANDLE mapfile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, L"EMCrashHandlerFileMapping");
	if (!mapfile)
	{
		WindowsError();
		fclose(logfile);
		return(1);
	}

	emstatus_t *emstatus = MapViewOfFile(mapfile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(emstatus_t));
	if (!emstatus)
	{
		WindowsError();
		CloseHandle(mapfile);
		fclose(logfile);
		return(1);
	}

	while (1)
	{
		if (emstatus->status == EMSTATUS_OK)
		{
			fprintf(logfile, "No errors occurred during engine runtime, exiting successfully\n");
			fflush(logfile);
			break;
		}

		if (emstatus->status == EMSTATUS_ERROR)
		{
			wchar_t wlpmsgbuf[1024] = { 0 };
			_snwprintf(wlpmsgbuf, 1023, L"An error has occurred during engine runtime, please check file [%s] for stack trace\n", logfullpath);
			wlpmsgbuf[1023] = L'\0';

			MessageBox(NULL, wlpmsgbuf, L"EMCrashHandler: ERROR", MB_OK | MB_ICONINFORMATION);
			fwprintf(logfile, wlpmsgbuf);

			for (int i=0; i<EMTRACE_MAX_LINES; i++)
			{
				if (emstatus->stacktrace[i][0] != '\0')
				{
					fprintf(logfile, "[EMSTATUS_ERROR] %s\n", emstatus->stacktrace[i]);
					fflush(logfile);
				}
			}

			break;
		}
	}

	UnmapViewOfFile(emstatus);
	CloseHandle(mapfile);
	fclose(logfile);

	return(0);
}
