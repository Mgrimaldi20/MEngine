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
	int newwidth = rect->right - rect->left;
	int newheight = rect->bottom - rect->top;

	win32state.fullwinwidth = newwidth;
	win32state.fullwinheight = newheight;
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

		case WM_SYSCHAR:
		case WM_CHAR:
			break;

		case WM_SYSKEYDOWN:
		case WM_KEYDOWN:
		{
			key = MapWin32Key(wparam);

			if (key == KEY_PRINTSCREEN)		// it should only send a key up but this is just in case
				break;

			break;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			key = MapWin32Key(wparam);

			if (key == KEY_PRINTSCREEN)
				break;

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
			key = MOUSE_LEFT;
			break;
		}

		case WM_RBUTTONDOWN:
		{
			key = MOUSE_RIGHT;
			break;
		}

		case WM_MBUTTONDOWN:
		{
			key = MOUSE_MIDDLE;
			break;
		}

		case WM_LBUTTONUP:
		{
			key = MOUSE_LEFT;
			break;
		}

		case WM_RBUTTONUP:
		{
			key = MOUSE_RIGHT;
			break;
		}

		case WM_MBUTTONUP:
		{
			key = MOUSE_MIDDLE;
			break;
		}

		case WM_XBUTTONDOWN:
		{
			if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				key = MOUSE_X1;

			else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
				key = MOUSE_X2;

			break;
		}

		case WM_XBUTTONUP:
		{
			if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				key = MOUSE_X1;

			else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
				key = MOUSE_X2;

			break;
		}

		case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			key = delta < 0 ? MOUSE_WHEELDOWN : MOUSE_WHEELUP;
			delta = abs(delta);

			while (delta-- > 0)
			{
				Common_Printf("Mouse wheel delta: %d\n", delta);	// just do this for now TODO: change later to do something with this
				Common_Printf("Mouse wheel key: %d\n", key);
			}

			break;
		}

		default:
			return(DefWindowProc(hwnd, umsg, wparam, lparam));
	}

	return(DefWindowProc(hwnd, umsg, wparam, lparam));
}
