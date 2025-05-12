#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
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
	(PWSTR)pcmdline;
	(int)ncmdshow;

	int argc = 0;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLine(), &argc);	// quirky function that will not return the exe name in the string if pcmdline is not NULL
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

	logfullpath[MAX_PATH - 1] = L'\0';
	FILE *logfile = _wfopen(logfullpath, L"w+");
	if (!logfile)
	{
		wchar_t errormsg[1024] = { 0 };
		_snwprintf(errormsg, 1023, L"Cannot open log file: %s, directory might not exist", logfullpath);
		errormsg[1023] = L'\0';

		MessageBox(NULL, errormsg, L"EMCrashHandler: Cannot Open Log File", MB_OK | MB_ICONINFORMATION);
		LocalFree(argv);
		return(1);
	}

	fprintf(logfile, "Starting crash handler\n");

	char logfullpatha[MAX_PATH] = { 0 };
	if (!WideCharToMultiByte(CP_UTF8, 0, logfullpath, -1, logfullpatha, MAX_PATH, NULL, NULL))
	{
		fprintf(logfile, "Failed to convert command line to UTF-8\n");
		fclose(logfile);
		LocalFree(argv);
		return(1);
	}

	fprintf(logfile, "Opening logs at: %s\n", logfullpatha);

	if (!pcmdline)
		fprintf(logfile, "Command line is NULL\n");

	fflush(logfile);

	LPSTR *argva = malloc(sizeof(LPSTR) * argc);
	if (!argva)
	{
		fprintf(logfile, "Failed to allocate memory for command line arguments\n");
		fclose(logfile);
		LocalFree(argv);
		return(1);
	}

	for (int i=0; i<argc; i++)
	{
		argva[i] = malloc(MAX_PATH);
		if (!argva[i])
		{
			fprintf(logfile, "Failed to allocate memory for argument %d\n", i);
			fclose(logfile);

			for (int j=0; j<i; j++)
				free(argva[j]);

			free(argva);
			LocalFree(argv);
			return(1);
		}

		if (!WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, argva[i], MAX_PATH, NULL, NULL))
		{
			fprintf(logfile, "Failed to convert command line argument to UTF-8\n");
			fclose(logfile);

			for (int j=0; j<=i; j++)
				free(argva[j]);

			free(argva);
			LocalFree(argv);
			return(1);
		}
	}

	fprintf(logfile, "Command line arguments: Argc = [%d]:\n", argc);
	for (int i=0; i<argc; i++)
	{
		fprintf(logfile, "\tArgv[%d]: %s\n", i, argva[i]);
		fflush(logfile);
	}

	LocalFree(argv);	// CommandLineToArgvW allocates memory for argv

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

	HANDLE connevent = OpenEvent(EVENT_ALL_ACCESS, FALSE, L"EMCrashHandlerConnectEvent");
	if (!connevent)
	{
		WindowsError();
		UnmapViewOfFile(emstatus);
		CloseHandle(mapfile);
		fclose(logfile);
		return(1);
	}

	fprintf(logfile, "\nWaiting for engine to connect...\n");

	if (WaitForSingleObject(connevent, INFINITE) == WAIT_OBJECT_0)
	{
		fprintf(logfile, "Engine connected\n");
		fflush(logfile);

		emstatus->connected = true;
	}

	while (1)
	{
		if (emstatus->userdata[0] != '\0')
		{
			fprintf(logfile, "Writing User Data:\n\t%s\n", emstatus->userdata);
			fflush(logfile);
			memset(emstatus->userdata, '\0', EMCH_MAX_USERDATA_SIZE);
		}

		if (emstatus->status == EMSTATUS_EXIT_OK)
		{
			fprintf(logfile, "\nNo errors occurred during engine runtime, exiting successfully\n");
			break;
		}

		if (emstatus->status == EMSTATUS_EXIT_ERROR)
		{
			fprintf(logfile, "\nThe engine has exited due to a known error, please check the main engine logs for information\n");
			break;
		}

		if (emstatus->status == EMSTATUS_EXIT_CRASH)
		{
			wchar_t wlpmsgbuf[1024] = { 0 };
			_snwprintf(wlpmsgbuf, 1023, L"\nAn error has occurred during engine runtime, please check file [%s] for stack trace\n", logfullpath);
			wlpmsgbuf[1023] = L'\0';

			MessageBox(NULL, wlpmsgbuf, L"EMCrashHandler: ERROR", MB_OK | MB_ICONINFORMATION);

			char msgbuf[1024] = { 0 };
			WideCharToMultiByte(CP_UTF8, 0, wlpmsgbuf, -1, msgbuf, 1023, NULL, NULL);
			msgbuf[1023] = '\0';

			fprintf(logfile, "\n[EMSTATUS_ERROR] | %s\n", msgbuf);

			for (int i=0; i<EMCH_MAX_FRAMES; i++)
			{
				if (emstatus->stacktrace[i][0] != '\0')
				{
					fprintf(logfile, "[EMSTATUS_ERROR] | %s\n", emstatus->stacktrace[i]);
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
