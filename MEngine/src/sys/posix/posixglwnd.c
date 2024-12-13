#include "common/common.h"
#include "posixlocal.h"
#include "renderer/renderer.h"

static bool initialized;

bool GLWnd_Init(glwndparams_t params)
{
	if (!glfwInit())
	{
		Log_WriteSeq(LOG_ERROR, "Failed to initialise GLFW");
		return(false);
	}

	GLFWmonitor *monitor = glfwGetPrimaryMonitor();
	if (!monitor)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get primary monitor");
		posixstate.fullscreen = false;
		return(false);
	}

	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	if (!mode)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get video mode");
		return(false);
	}

	posixstate.desktopwidth = mode->width;
	posixstate.desktopheight = mode->height;
	posixstate.desktoprefresh = mode->refreshRate;

	posixstate.fullscreen = true;

	if (!params.fullscreen)
	{
		posixstate.fullscreen = false;
		monitor = NULL;
	}

	// if the user has refresh, width, and height set to 0, use the desktop settings
	if (params.refreshrate == 0 || (params.refreshrate > posixstate.desktoprefresh))
		params.refreshrate = posixstate.desktoprefresh;

	if (params.width == 0 || (params.width > posixstate.desktopwidth))
	{
		params.width = posixstate.desktopwidth;
		glstate.width = posixstate.desktopwidth;
	}

	if (params.height == 0 || (params.height > posixstate.desktopheight))
	{
		params.height = posixstate.desktopheight;
		glstate.height = posixstate.desktopheight;
	}

	posixstate.window = glfwCreateWindow(320, 240, params.wndname, monitor, NULL);	// create with a small resolution at first
	if (!posixstate.window)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to create window");
		glfwTerminate();
		return(false);
	}

	glfwSetWindowSize(posixstate.window, params.width, params.height);

	// centre the window if not in fullscreen mode
	if (!params.fullscreen)
		glfwSetWindowPos(posixstate.window, (posixstate.desktopwidth - params.width) / 2, (posixstate.desktopheight - params.height) / 2);

	glfwMakeContextCurrent(posixstate.window);		// create the OpenGL context

	RegisterCallbacks(posixstate.window);

	Log_WriteSeq(LOG_INFO, "OpenGL initalised and created window");

	initialized = true;

	return(true);
}

void GLWnd_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down OpenGL system");

	if (posixstate.window)
	{
		glfwMakeContextCurrent(NULL);
		glfwDestroyWindow(posixstate.window);
		posixstate.window = NULL;
	}

	glfwTerminate();

	initialized = false;
}

bool GLWnd_ChangeScreenParams(glwndparams_t params)
{
	if (!posixstate.window)
	{
		Log_WriteSeq(LOG_ERROR, "Window not created, cannot change screen parameters");
		return(false);
	}

	GLFWmonitor *monitor = NULL;

	if (params.fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
		posixstate.fullscreen = true;

		if (!monitor)
		{
			Log_WriteSeq(LOG_WARN, "Failed to get primary monitor, cannot make fullscreen window, starting in windowed mode instead");
			posixstate.fullscreen = false;
			monitor = NULL;		// just in case
		}
	}

	glfwSetWindowMonitor(posixstate.window, monitor, 0, 0, params.width, params.height, params.refreshrate);

	if (!params.fullscreen)
		glfwSetWindowPos(posixstate.window, (posixstate.desktopwidth - params.width) / 2, (posixstate.desktopheight - params.height) / 2);

	return(true);
}

void GLWnd_SwapBuffers(void)
{
	glfwSwapBuffers(posixstate.window);
}

void GLWnd_SetVSync(int vsync)
{
	glfwSwapInterval(vsync);
	posixstate.swapinterval = vsync;
}

int GLWnd_GetVSync(void)
{
	return(posixstate.swapinterval);
}
