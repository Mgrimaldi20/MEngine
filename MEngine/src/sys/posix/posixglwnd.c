#include "common/common.h"
#include "posixlocal.h"
#include "renderer/renderer.h"

static bool initialized;

/*
* Function: GLWnd_Init
* Initializes the OpenGL window and context using GLFW for POSIX systems, also registers the window callbacks
* 
* 	params: The parameters to use for the window creation
* 
* Returns: A boolean if the initialization was successful or not
*/
bool GLWnd_Init(glwndparams_t params)
{
	if (initialized)
		return(true);

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

	RegisterCallbacks(posixstate.window);			// register all the window callbacks

	Log_WriteSeq(LOG_INFO, "OpenGL initalised and created window");

	initialized = true;

	return(true);
}

/*
* Function: GLWnd_Shutdown
* Shuts down the windowing system
*/
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

/*
* Function: GLWnd_ChangeScreenParams
* Changes the screen parameters of the window, for example resolution, refresh rate, and fullscreen modes
* 
* 	params: The parameters to change the screen to
* 
* Returns: A boolean if the operation was successful or not
*/
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

/*
* Function: GLWnd_SwapBuffers
* Swaps the front and back buffers of the window
*/
void GLWnd_SwapBuffers(void)
{
	glfwSwapBuffers(posixstate.window);
}

/*
* Function: GLWnd_SetVSync
* Sets the vertical sync of the window
* 
* 	vsync: The vertical sync value to set, 1 for on, 0 for off, as numbers increase a factor of the refresh rate is used
*/
void GLWnd_SetVSync(int vsync)
{
	glfwSwapInterval(vsync);
	posixstate.swapinterval = vsync;
}

/*
* Function: GLWnd_GetVSync
* Gets the vertical sync value of the window
* 
* Returns: The vertical sync value of the window
*/
int GLWnd_GetVSync(void)
{
	return(posixstate.swapinterval);
}
