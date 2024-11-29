#include <stdio.h>
#include <time.h>
#include "../../common/common.h"
#include "winlocal.h"
#include "../../renderer/renderer.h"
#include "wglext.h"		// for OpenGL Windows extensions

#define WINDOW_STYLE (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE | WS_SIZEBOX)
#define FS_WINDOW_STYLE (WS_POPUP | WS_VISIBLE | WS_SYSMENU)

typedef long long (*glwndproc_t)(void);

static const wchar_t *wndclassname = L"MEngine";
static const wchar_t *fakewndclassname = L"Fake_MEngine";

PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB;

PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
PFNWGLGETSWAPINTERVALEXTPROC wglGetSwapIntervalEXT;

PFNWGLGETPIXELFORMATATTRIBIVARBPROC wglGetPixelFormatAttribivARB;
PFNWGLGETPIXELFORMATATTRIBFVARBPROC wglGetPixelFormatAttribfvARB;
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;

PFNWGLCREATEPBUFFERARBPROC wglCreatePbufferARB;
PFNWGLGETPBUFFERDCARBPROC wglGetPbufferDCARB;
PFNWGLRELEASEPBUFFERDCARBPROC wglReleasePbufferDCARB;
PFNWGLDESTROYPBUFFERARBPROC wglDestroyPbufferARB;
PFNWGLQUERYPBUFFERARBPROC wglQueryPbufferARB;

PFNWGLBINDTEXIMAGEARBPROC wglBindTexImageARB;
PFNWGLRELEASETEXIMAGEARBPROC wglReleaseTexImageARB;
PFNWGLSETPBUFFERATTRIBARBPROC wglSetPbufferAttribARB;

static glwndproc_t GetWGLProcAddress(const char *extension)
{
	glwndproc_t proc = (glwndproc_t)wglGetProcAddress(extension);
	if (!proc)
	{
		Log_Write(LOG_INFO, "Could not get WGL proc address for %s", extension);
		return(NULL);
	}

	return(proc);
}

// done to get Windows OpenGL extensions, need to create a dummy context to do this
static LRESULT CALLBACK FakeWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (msg == WM_DESTROY)
		PostQuitMessage(0);

	if (msg != WM_CREATE)
		return(DefWindowProc(hwnd, msg, wparam, lparam));

	// little bit weird but it gets it set up
	static const PIXELFORMATDESCRIPTOR pfd =
	{
		.nSize = sizeof(pfd),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 32,
		.cRedBits = 0,
		.cRedShift = 0,
		.cGreenBits = 0,
		.cGreenBits = 0,
		.cBlueBits = 0,
		.cBlueShift = 0,
		.cAlphaBits = 8,
		.cAlphaShift = 0,
		.cAccumBits = 0,
		.cAccumRedBits = 0,
		.cAccumGreenBits = 0,
		.cAccumBlueBits = 0,
		.cAccumAlphaBits = 0,
		.cDepthBits = 24,
		.cStencilBits = 8,
		.cAuxBuffers = 0,
		.iLayerType = PFD_MAIN_PLANE,
		.bReserved = 0,
		.dwLayerMask = 0,
		.dwVisibleMask = 0,
		.dwDamageMask = 0
	};

	HDC hdc = GetDC(hwnd);

	int pixelformat = ChoosePixelFormat(hdc, &pfd);
	SetPixelFormat(hdc, pixelformat, &pfd);		// weirdly need this to get WGL extensions or the whole fake window thing wont work
	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);

	return(DefWindowProc(hwnd, msg, wparam, lparam));
}

static void GetWGLExtensions(HDC hdc)
{
	wglGetExtensionsStringARB = (PFNWGLGETEXTENSIONSSTRINGARBPROC)GetWGLProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtensionsStringARB)
	{
		win32state.extstr = (char *)wglGetExtensionsStringARB(hdc);

		static bool printed = false;
		if (!printed)
		{
			Log_WriteSeq(LOG_INFO, "WGL extensions: %s", win32state.extstr);	// just checking
			printed = true;
		}
	}

	else
		win32state.extstr = NULL;

	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)GetWGLProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT = (PFNWGLGETSWAPINTERVALEXTPROC)GetWGLProcAddress("wglGetSwapIntervalEXT");

	wglGetPixelFormatAttribivARB = (PFNWGLGETPIXELFORMATATTRIBIVARBPROC)GetWGLProcAddress("wglGetPixelFormatAttribivARB");
	wglGetPixelFormatAttribfvARB = (PFNWGLGETPIXELFORMATATTRIBFVARBPROC)GetWGLProcAddress("wglGetPixelFormatAttribfvARB");
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)GetWGLProcAddress("wglChoosePixelFormatARB");

	wglCreatePbufferARB = (PFNWGLCREATEPBUFFERARBPROC)GetWGLProcAddress("wglCreatePbufferARB");
	wglGetPbufferDCARB = (PFNWGLGETPBUFFERDCARBPROC)GetWGLProcAddress("wglGetPbufferDCARB");
	wglReleasePbufferDCARB = (PFNWGLRELEASEPBUFFERDCARBPROC)GetWGLProcAddress("wglReleasePbufferDCARB");
	wglDestroyPbufferARB = (PFNWGLDESTROYPBUFFERARBPROC)GetWGLProcAddress("wglDestroyPbufferARB");
	wglQueryPbufferARB = (PFNWGLQUERYPBUFFERARBPROC)GetWGLProcAddress("wglQueryPbufferARB");

	wglBindTexImageARB = (PFNWGLBINDTEXIMAGEARBPROC)GetWGLProcAddress("wglBindTexImageARB");
	wglReleaseTexImageARB = (PFNWGLRELEASETEXIMAGEARBPROC)GetWGLProcAddress("wglReleaseTexImageARB");
	wglSetPbufferAttribARB = (PFNWGLSETPBUFFERATTRIBARBPROC)GetWGLProcAddress("wglSetPbufferAttribARB");
}

static bool CreateFakeWindowExt(void)
{
	HWND hwnd = CreateWindowEx(
		0,
		fakewndclassname,
		L"Fake Window",
		WS_OVERLAPPEDWINDOW,
		0, 0, 1, 1,
		NULL,
		NULL,
		win32state.hinst,
		NULL
	);

	if (!hwnd)
	{
		Sys_Error("%s: Could not create fake window for WGL extensions", __func__);
		return(false);
	}

	HDC hdc = GetDC(hwnd);
	HGLRC hglrc = wglCreateContext(hdc);
	wglMakeCurrent(hdc, hglrc);

	GetWGLExtensions(hdc);

	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);

	DestroyWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return(true);
}

static bool InitOpenGL(glwndparams_t params)
{
	if (win32state.hdc != NULL)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Non-NULL HDC already exists: %lld", __func__, win32state.hdc);
		return(false);
	}

	win32state.hdc = GetDC(win32state.hwnd);
	if (!win32state.hdc)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not get HDC", __func__);
		return(false);
	}

	if (wglChoosePixelFormatARB && (params.multisamples > 1))
	{
		UINT numformats = 0;
		float fattributes[] = { 0, 0 };

		int iattributes[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 32,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
			WGL_SAMPLES_ARB, params.multisamples,
			0, 0
		};

		if (!wglChoosePixelFormatARB(win32state.hdc, iattributes, fattributes, 1, &win32state.pixelformat, &numformats))
		{
			Log_WriteSeq(LOG_ERROR, "%s: Could not choose pixel format", __func__);
			return(false);
		}
		
		Log_WriteSeq(LOG_INFO, "Using wglChoosePixelFormatARB, PIXELFORMAT %d selected", win32state.pixelformat);
	}

	else
	{
		PIXELFORMATDESCRIPTOR pfd =
		{
			.nSize = sizeof(pfd),
			.nVersion = 1,
			.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			.iPixelType = PFD_TYPE_RGBA,
			.cColorBits = 32,
			.cRedBits = 0,
			.cRedShift = 0,
			.cGreenBits = 0,
			.cGreenShift = 0,
			.cBlueBits = 0,
			.cBlueShift = 0,
			.cAlphaBits = 8,
			.cAlphaShift = 0,
			.cAccumBits = 0,
			.cAccumRedBits = 0,
			.cAccumGreenBits = 0,
			.cAccumBlueBits = 0,
			.cAccumAlphaBits = 0,
			.cDepthBits = 24,
			.cStencilBits = 8,
			.cAuxBuffers = 0,
			.iLayerType = PFD_MAIN_PLANE,
			.bReserved = 0,
			.dwLayerMask = 0,
			.dwVisibleMask = 0,
			.dwDamageMask = 0
		};

		win32state.pixelformat = ChoosePixelFormat(win32state.hdc, &pfd);
		Log_WriteSeq(LOG_INFO, "Using ChoosePixelFormat, PIXELFORMAT %d selected", win32state.pixelformat);
	}

	if (!win32state.pixelformat)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not choose Pixel Format", __func__);
		return(false);
	}

	if (!DescribePixelFormat(win32state.hdc, win32state.pixelformat, sizeof(win32state.pfd), &win32state.pfd))
	{
		Log_WriteSeq(LOG_ERROR, "%s: Failed to describe Pixel Format", __func__);
		return(false);
	}

	if (!SetPixelFormat(win32state.hdc, win32state.pixelformat, &win32state.pfd))
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not set Pixel Format", __func__);
		return(false);
	}

	// setup OpenGL stuff
	Log_WriteSeq(LOG_INFO, "Creating OpenGL context...");
	win32state.hglrc = wglCreateContext(win32state.hdc);
	if (!win32state.hglrc)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not create OpenGL context", __func__);
		return(false);
	}

	if (!wglMakeCurrent(win32state.hdc, win32state.hglrc))
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not make OpenGL context current", __func__);
		return(false);
	}

	return(true);
}

static void CreateWndClasses(void)
{
	if (win32state.wndclassregistered)
		return;

	WNDCLASSEX wc =
	{
		.cbSize = sizeof(wc),
		.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = MainWndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = win32state.hinst,
		.hIcon = 0,
		.hIconSm = 0,
		.hCursor = 0,
		.hbrBackground = 0,
		.lpszMenuName = 0,
		.lpszClassName = wndclassname
	};

	if (!RegisterClassEx(&wc))
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not register window class", __func__);
		win32state.wndclassregistered = false;
	}

	Log_WriteSeq(LOG_INFO, "Registered window class");

	wc = (WNDCLASSEX)
	{
		.cbSize = sizeof(wc),
		.style = 0,
		.lpfnWndProc = FakeWndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = win32state.hinst,
		.hIcon = 0,
		.hIconSm = 0,
		.hCursor = 0,
		.hbrBackground = 0,
		.lpszMenuName = 0,
		.lpszClassName = fakewndclassname
	};

	if (!RegisterClassEx(&wc))
	{
		Log_WriteSeq(LOG_ERROR, "%s: Could not register fake window class", __func__);
		win32state.wndclassregistered = false;
	}

	Log_WriteSeq(LOG_INFO, "Registered fake window class");
	win32state.wndclassregistered = true;
}

static bool GLCreateWindow(glwndparams_t params)
{
	long x, y, w, h;
	DWORD wndstyle = WINDOW_STYLE;
	DWORD exstyle = 0;

	if (params.fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		wndstyle = FS_WINDOW_STYLE;

		x = 0;
		y = 0;
		w = params.width;
		h = params.height;
	}

	else
	{
		RECT r =
		{
			.left = 0,
			.top = 0,
			.right = params.width,
			.bottom = params.height
		};

		// just in case
		wndstyle = WINDOW_STYLE;
		exstyle = 0;
		AdjustWindowRect(&r, wndstyle, false);

		w = r.right - r.left;
		h = r.bottom - r.top;

		RECT clntr;
		GetClientRect(GetDesktopWindow(), &clntr);

		clntr.left = (clntr.right / 2) - (params.width / 2);
		clntr.top = (clntr.bottom / 2) - (params.height / 2);

		x = clntr.left;
		y = clntr.top;

		if ((x + w) > win32state.desktopwidth)
			x = win32state.desktopwidth - w;

		if ((y + h) > win32state.desktopheight)
			y = win32state.desktopheight - h;

		if (x < 0)
			x = 0;

		if (y < 0)
			y = 0;
	}

	wchar_t wwndname[MAX_WIN_NAME];
	if (!MultiByteToWideChar(CP_UTF8, 0, params.wndname, MAX_WIN_NAME, wwndname, MAX_WIN_NAME))
		return(false);

	wwndname[MAX_WIN_NAME - 1] = L'\0';

	win32state.hwnd = CreateWindowEx(
		exstyle,
		wndclassname,
		wwndname,
		wndstyle,
		x, y, w, h,
		NULL,
		NULL,
		win32state.hinst,
		NULL
	);

	if (!win32state.hwnd)
		return(false);

	ShowWindow(win32state.hwnd, SW_SHOW);
	UpdateWindow(win32state.hwnd);

	if (!InitOpenGL(params))
		Sys_Error("%s: Could not initialise OpenGL", __func__);

	SetForegroundWindow(win32state.hwnd);
	SetFocus(win32state.hwnd);

	return(true);
}

static bool SetFullScreen(glwndparams_t params)
{
	// check first if the selected mode is even valid
	DEVMODE dmtest;
	int mode = 0;
	bool validmode = false;

	memset(&dmtest, 0, sizeof(dmtest));

	while (EnumDisplaySettings(NULL, mode, &dmtest))
	{
		if ((int)dmtest.dmPelsWidth == params.width &&
			(int)dmtest.dmPelsHeight == params.height &&
			(int)dmtest.dmBitsPerPel == 32 &&
			(int)dmtest.dmDisplayFrequency == params.refreshrate)
		{
			Log_WriteSeq(LOG_INFO, "Selected display mode %dx%d @ %dHz is valid", dmtest.dmPelsWidth, dmtest.dmPelsHeight, dmtest.dmDisplayFrequency);
			validmode = true;
			break;
		}

		mode++;
	}

	if (!validmode)
	{
		Log_WriteSeq(LOG_ERROR, "%s: No valid display mode found", __func__);
		return(false);
	}

	DEVMODE dm;
	memset(&dm, 0, sizeof(dm));

	// try to set the users settings
	EnumDisplaySettings(NULL, 0, &dm);
	dm.dmPelsWidth = params.width;
	dm.dmPelsHeight = params.height;
	dm.dmBitsPerPel = 32;
	dm.dmDisplayFrequency = params.refreshrate;
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL | DM_DISPLAYFREQUENCY;

	if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
	{
		Log_WriteSeq(LOG_INFO, "Changed display settings to %dx%d @ %dHz", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
		return(true);
	}

	// if the user settings fail, try to get the next best thing
	mode = 0;

	memset(&dm, 0, sizeof(dm));

	while (EnumDisplaySettings(NULL, mode, &dm))
	{
		if ((int)dm.dmPelsWidth >= params.width &&
			(int)dm.dmPelsHeight >= params.height &&
			(int)dm.dmBitsPerPel == 32)
		{
			if (ChangeDisplaySettings(&dm, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL)
			{
				Log_WriteSeq(LOG_INFO, "Changed display settings to %dx%d @ %dHz", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
				return(true);
			}
		}

		mode++;
	}

	Log_WriteSeq(LOG_ERROR, "%s: Could not set fullscreen mode", __func__);
	return(false);
}

bool GLWnd_Init(glwndparams_t params)
{
	HDC hdc = GetDC(GetDesktopWindow());
	if (!hdc)
		WindowsError();

	win32state.desktopwidth = GetDeviceCaps(hdc, HORZRES);
	win32state.desktopheight = GetDeviceCaps(hdc, VERTRES);
	win32state.desktoprefresh = GetDeviceCaps(hdc, VREFRESH);
	win32state.desktopbpp = GetDeviceCaps(hdc, BITSPIXEL);

	ReleaseDC(GetDesktopWindow(), hdc);

	if (win32state.desktopbpp < 32 && !params.fullscreen)
	{
		Log_WriteSeq(LOG_ERROR, "%s: Desktop is not 32bpp, windowed mode requires 32bpp", __func__);
		return(false);
	}

	// if the user has refresh, width, and height set to 0, use the desktop settings
	if ((params.refreshrate == 0) || (params.refreshrate > win32state.desktoprefresh))
		params.refreshrate = win32state.desktoprefresh;

	if ((params.width == 0) || (params.width > win32state.desktopwidth))
	{
		params.width = win32state.desktopwidth;
		glstate.width = win32state.desktopwidth;
	}

	if ((params.height == 0) || (params.height > win32state.desktopheight))
	{
		params.height = win32state.desktopheight;
		glstate.height = win32state.desktopheight;
	}

	CreateWndClasses();
	if (!win32state.wndclassregistered)
		WindowsError();

	// create fake window to get WGL extensions, very weird but its how DOOM 3 does it so it must work
	if (!CreateFakeWindowExt())
		return(false);

	if (params.fullscreen)
	{
		if (!SetFullScreen(params))
			return(false);

		win32state.fullscreen = true;
	}

	if (!GLCreateWindow(params))
		WindowsError();

	GetWGLExtensions(win32state.hdc);	// get WGL extensions for real window

	Log_WriteSeq(LOG_INFO, "OpenGL initalised and created window");
	return(true);
}

void GLWnd_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down OpenGL system");

	wglMakeCurrent(NULL, NULL);

	if (win32state.hglrc)
	{
		wglDeleteContext(win32state.hglrc);
		win32state.hglrc = NULL;
	}

	if (win32state.hdc)
	{
		ReleaseDC(win32state.hwnd, win32state.hdc);
		win32state.hdc = NULL;
	}

	if (win32state.hwnd)
	{
		ShowWindow(win32state.hwnd, SW_HIDE);
		DestroyWindow(win32state.hwnd);
		win32state.hwnd = NULL;
	}

	UnregisterClass(fakewndclassname, win32state.hinst);
	UnregisterClass(wndclassname, win32state.hinst);
}

bool GLWnd_ChangeScreenParams(glwndparams_t params)
{
	long x, y, w, h;
	DWORD wndstyle;
	DWORD exstyle;
	DEVMODE dm;

	memset(&dm, 0, sizeof(dm));

	dm.dmSize = sizeof(dm);
	dm.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (params.refreshrate > 0)
	{
		dm.dmDisplayFrequency = params.refreshrate;
		dm.dmFields |= DM_DISPLAYFREQUENCY;
	}

	win32state.fullscreen = params.fullscreen;

	if (params.fullscreen)
	{
		exstyle = WS_EX_TOPMOST;
		wndstyle = FS_WINDOW_STYLE;

		x = 0;
		y = 0;
		w = params.width;
		h = params.height;

		dm.dmPelsWidth = params.width;
		dm.dmPelsHeight = params.height;
		dm.dmBitsPerPel = 32;

		SetWindowLong(win32state.hwnd, GWL_STYLE, wndstyle);
		SetWindowLong(win32state.hwnd, GWL_EXSTYLE, exstyle);
	}

	else
	{
		RECT r =
		{
			.left = 0,
			.top = 0,
			.right = params.width,
			.bottom = params.height
		};

		wndstyle = WINDOW_STYLE;
		exstyle = 0;
		AdjustWindowRect(&r, wndstyle, false);

		w = r.right - r.left;
		h = r.bottom - r.top;

		RECT clntr;
		GetClientRect(GetDesktopWindow(), &clntr);

		clntr.left = (clntr.right / 2) - (params.width / 2);
		clntr.top = (clntr.bottom / 2) - (params.height / 2);

		x = clntr.left;
		y = clntr.top;

		if ((x + w) > win32state.desktopwidth)
			x = win32state.desktopwidth - w;

		if ((y + h) > win32state.desktopheight)
			y = win32state.desktopheight - h;

		if (x < 0)
			x = 0;

		if (y < 0)
			y = 0;

		dm.dmPelsWidth = win32state.desktopwidth;
		dm.dmPelsHeight = win32state.desktopheight;
		dm.dmBitsPerPel = win32state.desktopbpp;

		SetWindowLong(win32state.hwnd, GWL_STYLE, wndstyle);
		SetWindowLong(win32state.hwnd, GWL_EXSTYLE, exstyle);
	}

	SetWindowPos(
		win32state.hwnd,
		params.fullscreen ? HWND_TOPMOST : HWND_NOTOPMOST,
		x, y, w, h,
		params.fullscreen ? SWP_NOSIZE | SWP_NOMOVE : SWP_SHOWWINDOW
	);

	bool ret = ChangeDisplaySettings(&dm, params.fullscreen ? CDS_FULLSCREEN : 0) == DISP_CHANGE_SUCCESSFUL;
	Log_WriteSeq(LOG_INFO, "Changed display settings to %dx%d @ %dHz", dm.dmPelsWidth, dm.dmPelsHeight, dm.dmDisplayFrequency);
	return(ret);
}

void GLWnd_SwapBuffers(void)
{
	SwapBuffers(win32state.hdc);
}

void GLWnd_SetVSync(int vsync)
{
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(vsync);
}

int GLWnd_GetVSync(void)
{
	if (wglGetSwapIntervalEXT)
		return(wglGetSwapIntervalEXT());

	return(-1);	// if the extension is not available
}
