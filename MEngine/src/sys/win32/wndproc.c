#include <stdio.h>
#include <time.h>
#include <math.h>
#include "common/common.h"
#include "winlocal.h"
#include "renderer/renderer.h"

/*
* Function: ResizeWindow
* Resizes the window viewport to match the new window size, gets the new client region dimensions
* 
*	hwnd: The window handle to resize
*/
static void ResizeWindow(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);

	glstate.width = (long)(rect.right - rect.left);
	glstate.height = (long)(rect.bottom - rect.top);

	if (glstate.width <= 0 || glstate.height <= 0)
		return;

	Cmd_BufferCommand(CMD_EXEC_NOW, "sizeviewport");
}

/*
* Function: WindowSizing
* Resizes the window to the minimum size if the window is too small, handles the window sizing messages
* 
*	wparam: The window message parameter
*/
static void WindowSizing(WPARAM wparam, RECT *rect)
{
	if (!glstate.initialized || glstate.width <=0 || glstate.height <= 0)
		return;

	int minwidth = Render_GetMinWidth();
	int minheight = Render_GetMinHeight();

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

/*
* Function: MainWndProc
* The main window procedure for the engine, handles all the window messages including native events,
* uses the default window procedure for unhandled messages
* 
*	hwnd: The window handle
*	umsg: The message
*	wparam: The high word of the message parameter
*	lparam: The low word of the message parameter
* 
* Returns: The result of the message processing
*/
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	keycode_t key = KEY_UNKNOWN;

	switch (umsg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_KILLFOCUS:
			Input_ClearKeyStates();
			break;

		case WM_ACTIVATE:
			if (LOWORD(wparam) == WA_ACTIVE || LOWORD(wparam) == WA_CLICKACTIVE)
				Input_ClearKeyStates();

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

			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
			break;
		}

		case WM_SYSKEYUP:
		case WM_KEYUP:
		{
			key = MapWin32Key(wparam);

			if (key == KEY_PRINTSCREEN)
				break;

			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			break;
		}

		case WM_MOUSEMOVE:		// relative to the top-left corner of the client region
		{
			POINTS pt = MAKEPOINTS(lparam);
			int posx = pt.x;
			int posy = pt.y;

			if (posx >= 0 && posx < glstate.width && posy >= 0 && posy < glstate.height)	// only update if the mouse is inside the framebuffer
				Event_QueueEvent(EVENT_MOUSE, posx, posy);	// event vars 1 and 2 become the x and y pos of the mouse

			break;
		}

		case WM_LBUTTONDOWN:	// mouse events have a keycode, so they still are key events
		{
			key = MOUSE_LEFT;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
			break;
		}

		case WM_RBUTTONDOWN:
		{
			key = MOUSE_RIGHT;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
			break;
		}

		case WM_MBUTTONDOWN:
		{
			key = MOUSE_MIDDLE;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
			break;
		}

		case WM_LBUTTONUP:
		{
			key = MOUSE_LEFT;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			break;
		}

		case WM_RBUTTONUP:
		{
			key = MOUSE_RIGHT;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			break;
		}

		case WM_MBUTTONUP:
		{
			key = MOUSE_MIDDLE;
			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			break;
		}

		case WM_XBUTTONDOWN:
		{
			if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				key = MOUSE_X1;

			else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
				key = MOUSE_X2;

			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
			break;
		}

		case WM_XBUTTONUP:
		{
			if (GET_XBUTTON_WPARAM(wparam) == XBUTTON1)
				key = MOUSE_X1;

			else if (GET_XBUTTON_WPARAM(wparam) == XBUTTON2)
				key = MOUSE_X2;

			Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			break;
		}

		case WM_MOUSEWHEEL:		// mouse wheel also has a keycode, so its a key event
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wparam);
			key = delta < 0 ? MOUSE_WHEELDOWN : MOUSE_WHEELUP;
			delta = abs(delta);

			while (delta-- > 0)
			{
				Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYDOWN);
				Event_QueueEvent(EVENT_KEY, key, EVENT_STATE_KEYUP);
			}

			break;
		}

		default:
			return(DefWindowProc(hwnd, umsg, wparam, lparam));
	}

	return(DefWindowProc(hwnd, umsg, wparam, lparam));
}
