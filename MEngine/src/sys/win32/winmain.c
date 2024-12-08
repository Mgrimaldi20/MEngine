#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common/common.h"

#include "winlocal.h"

#pragma warning(disable: 28251)		// disables windows annotations warning

static FILE *outfp, *infp, *errfp;

win32vars_t win32state;

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, PWSTR pcmdline, int ncmdshow)
{
	win32state = (win32vars_t){ 0 };

	if (hprevinst)
		return(1);

	win32state.hinst = hinst;
	win32state.pcmdline = pcmdline;
	win32state.ncmdshow = ncmdshow;

	win32state.conshow = false;

#if defined(MENGINE_DEBUG)
	win32state.conshow = true;
#endif

	if (!GetConsoleWindow())
	{
		InitConsole();
		HideConsole();
	}

	if (win32state.conshow)
		ShowConsole();
	
	if (!Common_Init())
	{
		Common_Shutdown();
		ShutdownConsole();
		return(1);
	}

	MSG msg;
	while (1)
	{
		while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage(&msg, NULL, 0, 0))
			{
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

	Common_Shutdown();
	ShutdownConsole();

	exit(1);
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
		Log_WriteSeq(LOG_ERROR, "Failed to free the console");	// just log it and move on, no need to force exit, this is prolly happening during shutdown anyway
		return;
	}
}

void ShowConsole(void)
{
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}

void HideConsole(void)
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}
