#include <stdio.h>
#include <time.h>
#include <math.h>
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
	mousecode_t code = MOUSE_UNKNOWN;

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

		case WM_SYSCHAR:
		case WM_CHAR:
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			key = MapWin32Key(wparam);

			if (key == KEY_PRINTSCREEN)		// it should only send a key up but this is just in case
				break;

#if defined(MENGINE_DEBUG)
			Common_Printf("%u: Key down: Translated: %d, Wparam: %llu\n", umsg, key, wparam);
#endif
			break;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			key = MapWin32Key(wparam);

			if (key == KEY_PRINTSCREEN)
				break;

#if defined(MENGINE_DEBUG)
			Common_Printf("%u: Key up: Translated: %d, Wparam: %llu\n", umsg, key, wparam);
#endif
			break;
		}

		case WM_MOUSEMOVE:
		{
			POINTS pt = MAKEPOINTS(lparam);
			int posx = pt.x;
			int posy = pt.y;

			if (posx >= 0 && posx < glstate.width && posy >= 0 && posy < glstate.height)	// only update if the mouse is inside the framebuffer
			{
				Common_Printf("Mouse moved X: %d\n", posx);	// just do this for now TODO: change later to do something with this
				Common_Printf("Mouse moved Y: %d\n", posy);
			}

			break;
		}

		case WM_LBUTTONDOWN:
		{
			code = MOUSE_LEFT;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button down: %d\n", code);
#endif
			break;
		}

		case WM_RBUTTONDOWN:
		{
			code = MOUSE_RIGHT;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button down: %d\n", code);
#endif
			break;
		}

		case WM_MBUTTONDOWN:
		{
			code = MOUSE_MIDDLE;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button down: %d\n", code);
#endif
			break;
		}

		case WM_LBUTTONUP:
		{
			code = MOUSE_LEFT;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button up: %d\n", code);
#endif
			break;
		}

		case WM_RBUTTONUP:
		{
			code = MOUSE_RIGHT;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button up: %d\n", code);
#endif
			break;
		}

		case WM_MBUTTONUP:
		{
			code = MOUSE_MIDDLE;

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button up: %d\n", code);
#endif
			break;
		}

		case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			code = delta < 0 ? MOUSE_WHEELDOWN : MOUSE_WHEELUP;
			delta = abs(delta);

			while (delta-- > 0)
			{
				Common_Printf("Mouse wheel delta: %d\n", delta);	// just do this for now TODO: change later to do something with this
				Common_Printf("Mouse wheel code: %d\n", code);
			}

			break;
		}

		default:
			return(DefWindowProc(hwnd, umsg, wparam, lparam));
	}

	return(DefWindowProc(hwnd, umsg, wparam, lparam));
}
