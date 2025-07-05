#pragma once

#if !defined(MENGINE_PLATFORM_WINDOWS)
#error Include is for Windows only
#endif

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdbool.h>
#include "common/keycodes.h"

#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE | WS_SIZEBOX)

typedef struct
{
	bool errorindicator;
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
	int fullwinwidth;		// full windows size including decorations
	int fullwinheight;
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
void OpenConsoleFiles(void);
void CloseConsoleFiles(void);
void ShowConsole(void);
void HideConsole(void);

keycode_t MapWin32Key(WPARAM key);
