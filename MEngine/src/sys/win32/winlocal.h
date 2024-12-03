#pragma once

#if !defined(MENGINE_PLATFORM_WINDOWS)
#error Include is for Windows only
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct
{
	bool wndclassregistered;
	bool fullscreen;
	bool conshow;
	char *extstr;
	int ncmdshow;
	int pixelformat;
	long desktopwidth;		// get all the desktop params
	long desktopheight;
	int desktopbpp;
	int desktoprefresh;
	HWND hwnd;
	HINSTANCE hinst;
	HDC hdc;
	HGLRC hglrc;
	PWSTR pcmdline;
	OSVERSIONINFOEX osver;
	PIXELFORMATDESCRIPTOR pfd;
} win32vars_t;

extern win32vars_t win32state;

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);

void WindowsError(void);
void InitConsole(void);
void ShutdownConsole(void);
void ShowConsole(void);
void HideConsole(void);
