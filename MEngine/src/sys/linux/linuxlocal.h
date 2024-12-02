#pragma once

#if !defined(__linux__) && !defined(__APPLE__)
#error Include is for Linux and Apple only
#endif

#include <stdbool.h>
#include <sys/utsname.h>
#include "GLFW/glfw3.h"

typedef struct
{
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
