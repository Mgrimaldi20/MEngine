#include "common/common.h"
#include "linuxlocal.h"
#include "renderer/renderer.h"

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
		linuxstate.fullscreen = false;
		return(false);
	}

	const GLFWvidmode *mode = glfwGetVideoMode(monitor);
	if (!mode)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get video mode");
		return(false);
	}

	linuxstate.desktopwidth = mode->width;
	linuxstate.desktopheight = mode->height;
	linuxstate.desktoprefresh = mode->refreshRate;

	linuxstate.fullscreen = true;

	if (!params.fullscreen)
	{
		linuxstate.fullscreen = false;
		monitor = NULL;
	}

	// if the user has refresh, width, and height set to 0, use the desktop settings
	if (params.refreshrate == 0 || (params.refreshrate > linuxstate.desktoprefresh))
		params.refreshrate = linuxstate.desktoprefresh;

	if (params.width == 0 || (params.width > linuxstate.desktopwidth))
	{
		params.width = linuxstate.desktopwidth;
		glstate.width = linuxstate.desktopwidth;
	}

	if (params.height == 0 || (params.height > linuxstate.desktopheight))
	{
		params.height = linuxstate.desktopheight;
		glstate.height = linuxstate.desktopheight;
	}

	linuxstate.window = glfwCreateWindow(320, 240, params.wndname, monitor, NULL);	// create with a small resolution at first
	if (!linuxstate.window)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to create window");
		glfwTerminate();
		return(false);
	}

	glfwSetWindowSize(linuxstate.window, params.width, params.height);

	// centre the window if not in fullscreen mode
	if (!params.fullscreen)
		glfwSetWindowPos(linuxstate.window, (linuxstate.desktopwidth - params.width) / 2, (linuxstate.desktopheight - params.height) / 2);

	glfwMakeContextCurrent(linuxstate.window);		// create the OpenGL context

	RegisterCallbacks(linuxstate.window);

	Log_WriteSeq(LOG_INFO, "OpenGL initalised and created window");
	return(true);
}

void GLWnd_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down OpenGL system");

	if (linuxstate.window)
	{
		glfwDestroyWindow(linuxstate.window);
		linuxstate.window = NULL;
	}

	glfwTerminate();
}

bool GLWnd_ChangeScreenParams(glwndparams_t params)
{
	if (!linuxstate.window)
	{
		Log_WriteSeq(LOG_ERROR, "Window not created, cannot change screen parameters");
		return(false);
	}

	GLFWmonitor *monitor = NULL;

	if (params.fullscreen)
	{
		monitor = glfwGetPrimaryMonitor();
		linuxstate.fullscreen = true;

		if (!monitor)
		{
			Log_WriteSeq(LOG_WARN, "Failed to get primary monitor, cannot make fullscreen window, starting in windowed mode instead");
			linuxstate.fullscreen = false;
			monitor = NULL;		// just in case
		}
	}

	glfwSetWindowMonitor(linuxstate.window, monitor, 0, 0, params.width, params.height, params.refreshrate);

	if (!params.fullscreen)
		glfwSetWindowPos(linuxstate.window, (linuxstate.desktopwidth - params.width) / 2, (linuxstate.desktopheight - params.height) / 2);

	return(true);
}

void GLWnd_SwapBuffers(void)
{
	if (!glfwWindowShouldClose(linuxstate.window))
		glfwSwapBuffers(linuxstate.window);
}

void GLWnd_SetVSync(int vsync)
{
	glfwSwapInterval(vsync);
	linuxstate.swapinterval = vsync;
}

int GLWnd_GetVSync(void)
{
	return(linuxstate.swapinterval);
}
