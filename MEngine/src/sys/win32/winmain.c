#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#pragma warning(disable: 28251)		// disables windows annotations warning

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h>
#include "../../common/common.h"
#include "winlocal.h"

static FILE *outfp, *infp, *errfp;

win32vars_t win32state;

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, PWSTR pcmdline, int ncmdshow)
{
	if (hprevinst)
		return(1);

	win32state.hinst = hinst;
	win32state.pcmdline = pcmdline;
	win32state.ncmdshow = ncmdshow;

	win32state.conshow = true;
	if (!GetConsoleWindow())
	{
		InitConsole();
		HideConsole();
	}

	char cmdline[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS];
	ProcessCommandLine(pcmdline, cmdline);
	
	if (!Common_Init(cmdline))
	{
		Common_Shutdown();
		ShutdownConsole();
		return(1);
	}

	Log_WriteSeq(LOG_INFO, "Engine Initialized successfully...");

	if (Common_HelpMode())
	{
		if (win32state.conshow)
		{
			Common_PrintHelpMsg();
			Common_Shutdown();
			return(0);
		}

		else
			ShowConsole();

		Common_PrintHelpMsg();

		volatile bool running = true;
		while (running)
		{
			if (_kbhit())
				running = false;
		}

		PostQuitMessage(0);
	}

	MSG msg;
	while (1)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
			{
				Log_WriteSeq(LOG_INFO, "Engine shutting down...");
				Common_Shutdown();
				ShutdownConsole();

				return(0);
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		Common_Frame();
	}

	return(1);
}

void WindowsError(void)
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

	MessageBox(NULL, wlpmsgbuf, L"GetLastError", MB_OK | MB_ICONINFORMATION);
	LocalFree(wlpmsgbuf);
}

void ProcessCommandLine(PWSTR cmdline, char cmdout[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS])
{
	wchar_t *saveptr = NULL;
	int i = 0;

	wchar_t *token = wcstok(cmdline, L" -", &saveptr);
	while (token != NULL)
	{
		WideCharToMultiByte(CP_UTF8, 0, token, -1, cmdout[i], MAX_CMDLINE_ARGS, NULL, NULL);
		token = wcstok(NULL, L" -", &saveptr);
		i++;
	}
}

void InitConsole(void)
{
	if (!AllocConsole())
	{
		WindowsError();
		return;
	}

	freopen_s(&outfp, "CONOUT$", "w", stdout);
	freopen_s(&errfp, "CONOUT$", "w", stderr);
	freopen_s(&infp, "CONIN$", "r", stdin);

	if (!outfp || !errfp || !infp)
	{
		if (outfp)
			fclose(outfp);

		if (errfp)
			fclose(errfp);

		if (infp)
			fclose(infp);

		return;
	}

	printf("Opening debugging console\n");
}

void ShutdownConsole(void)
{
	printf("\nClosing debugging console\n");

	if (outfp)
		fclose(outfp);

	if (errfp)
		fclose(errfp);

	if (infp)
		fclose(infp);

	if (!FreeConsole())
	{
		WindowsError();
		return;
	}
}

void ShowConsole(void)
{
	if (ShowWindow(GetConsoleWindow(), SW_SHOW))
		win32state.conshow = true;
}

void HideConsole(void)
{
	if (ShowWindow(GetConsoleWindow(), SW_HIDE))
		win32state.conshow = false;
}
