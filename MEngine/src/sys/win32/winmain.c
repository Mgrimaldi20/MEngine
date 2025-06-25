#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "common/common.h"

#include "winlocal.h"

#pragma warning(disable: 28251)		// disables windows annotations warning

static FILE *outfp;
static FILE *infp;
static FILE *errfp;

win32vars_t win32state;

int WINAPI wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, PWSTR pcmdline, int ncmdshow)
{
	win32state = (win32vars_t){ 0 };

	if (hprevinst)
		return(1);

	win32state.hinst = hinst;
	win32state.pcmdline = pcmdline;
	win32state.ncmdshow = ncmdshow;

	win32state.conshow = false;		// just use this to control console creation

	//win32state.pcmdline = L"-help";

#if defined(MENGINE_DEBUG)
	win32state.conshow = true;
#endif

	if (win32state.conshow)
	{
		if (!GetConsoleWindow())
		{
			InitConsole();
			HideConsole();
		}

		ShowConsole();
	}
	
	if (!Common_Init())
	{
		Common_Shutdown();

		if (win32state.conshow)
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

				if (win32state.conshow)
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

/*
* Function: WindowsError
* Displays a message box with the last windows error, used for Win32 API functions, used for fatal Windows errors
*/
void WindowsError(void)
{
	win32state.errorindicator = true;

	LPVOID wlpmsgbuf = NULL;

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

	if (win32state.conshow)
		ShutdownConsole();

	exit(1);
}

/*
* Function: InitConsole
* Initializes the debugging console
*/
void InitConsole(void)
{
	if (!AllocConsole())
	{
		win32state.conshow = false;
		WindowsError();
		return;
	}

	errno_t err = 0;

	err = freopen_s(&outfp, "CONOUT$", "w", stdout);
	if (err)
		Common_Errorf("Failed to redirect stdout to console");

	err = freopen_s(&errfp, "CONOUT$", "w", stderr);
	if (err)
		Common_Errorf("Failed to redirect stderr to console");

	err = freopen_s(&infp, "CONIN$", "r", stdin);
	if (err)
		Common_Errorf("Failed to redirect stdin to console");

	if (!outfp || !errfp || !infp)
	{
		if (outfp)
			fclose(outfp);

		if (errfp)
			fclose(errfp);

		if (infp)
			fclose(infp);

		win32state.conshow = false;

		return;
	}

	Common_Printf("Opening debugging console");
}

/*
* Function: ShutdownConsole
* Shuts down the debugging console
*/
void ShutdownConsole(void)
{
	Common_Printf("\nClosing debugging console");

	if (outfp)
		fclose(outfp);

	if (errfp)
		fclose(errfp);

	if (infp)
		fclose(infp);

	if (!FreeConsole())
	{
		Common_Errorf("Failed to free the console");	// just log it and move on, no need to force exit, this is prolly happening during shutdown anyway
		return;
	}
}

/*
* Function: ShowConsole
* Shows the debugging console
*/
void ShowConsole(void)
{
	ShowWindow(GetConsoleWindow(), SW_SHOW);
}

/*
* Function: HideConsole
* Hides the debugging console from view, but still allows output
*/
void HideConsole(void)
{
	ShowWindow(GetConsoleWindow(), SW_HIDE);
}
