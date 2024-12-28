#include <stdio.h>
#include <time.h>
#include "common/common.h"
#include "winlocal.h"
#include "renderer/renderer.h"

static void ResizeWindow(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);

	glstate.width = (long)(rect.right - rect.left);
	glstate.height = (long)(rect.bottom - rect.top);

	glstate.viewportsized = true;
}

static void WindowSizing(WPARAM wparam, RECT *rect)
{
	if (!glstate.initialized || glstate.width <=0 || glstate.height <= 0)
		return;

	int minwidth = 320;
	int minheight = 240;

	if (rect->right - rect->left < minwidth)
	{
		if (wparam == WMSZ_LEFT || wparam == WMSZ_TOPLEFT || wparam == WMSZ_BOTTOMLEFT)
			rect->left = rect->right - minwidth;

		else
			rect->right = rect->left + minwidth;
	}

	if (rect->bottom - rect->top < minheight)
	{
		if (wparam == WMSZ_TOP || wparam == WMSZ_TOPLEFT || wparam == WMSZ_TOPRIGHT)
			rect->top = rect->bottom - minheight;

		else
			rect->bottom = rect->top + minheight;
	}

	// get new the full window size including decorations
	int newwidth = rect->right - rect->left;;
	int newheight = rect->bottom - rect->top;

	(int)newwidth;
	(int)newheight;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	keycode_t key = KEY_UNKNOWN;

	switch (umsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_SIZE:
			ResizeWindow(hwnd);
			break;

		case WM_SIZING:
			WindowSizing(wparam, (RECT *)lparam);
			break;

		case WM_SYSCOMMAND:
			if (wparam == SC_SCREENSAVE || wparam == SC_KEYMENU)
				return(0);
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			key = MapWin32Key(wparam);

#if defined(MENGINE_DEBUG)
			printf("%u: Key down: Translated: %d, Wparam: %llu\n", umsg, key, wparam);
#endif
			break;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			key = MapWin32Key(wparam);

#if defined(MENGINE_DEBUG)
			printf("%u: Key up: Translated: %d, Wparam: %llu\n", umsg, key, wparam);
#endif
			break;
		}

		default:
			return(DefWindowProc(hwnd, umsg, wparam, lparam));
	}

	return(DefWindowProc(hwnd, umsg, wparam, lparam));
}
