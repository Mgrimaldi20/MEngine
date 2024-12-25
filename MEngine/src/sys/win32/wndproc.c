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

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	switch (umsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_SIZE:
			ResizeWindow(hwnd);
			break;

		case WM_SYSCOMMAND:
			if (wparam == SC_SCREENSAVE || wparam == SC_KEYMENU)
				return(0);
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
			break;

		case WM_SYSKEYUP:
		case WM_KEYUP:
			break;

		default:
			return(DefWindowProc(hwnd, umsg, wparam, lparam));
	}

	return(DefWindowProc(hwnd, umsg, wparam, lparam));
}
