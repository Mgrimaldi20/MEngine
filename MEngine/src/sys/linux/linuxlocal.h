#pragma once

#if !defined(MENGINE_PLATFORM_UNIX)
#error Include is for Linux and Apple only
#endif

#include <stdbool.h>
#include <sys/utsname.h>
#include "GLFW/glfw3.h"

typedef struct
{
	bool windowtoclose;
	bool fullscreen;
	int swapinterval;
	int desktopwidth;		// get all the desktop params
	int desktopheight;
	int desktoprefresh;
	int argc;
	char **argv;
	struct utsname osinfo;
	GLFWwindow *window;
} linuxvars_t;

extern linuxvars_t linuxstate;

void RegisterCallbacks(GLFWwindow *window);
