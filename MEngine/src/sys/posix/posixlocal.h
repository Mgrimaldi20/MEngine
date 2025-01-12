#pragma once

#if !defined(MENGINE_PLATFORM_LINUX) && !defined(MENGINE_PLATFORM_MACOS)
#error Include is for Linux and Apple only
#endif

#include <stdbool.h>
#include <sys/utsname.h>
#include <sys/resource.h>
#include "GLFW/glfw3.h"

#include "common/keycodes.h"

typedef struct
{
	bool initialized;
	bool fullscreen;
	int swapinterval;
	int desktopwidth;		// get all the desktop params
	int desktopheight;
	int desktoprefresh;
	int argc;
	char **argv;
	struct utsname osinfo;
	GLFWwindow *window;
} posixvars_t;

extern posixvars_t posixstate;

void RegisterCallbacks(GLFWwindow *window);

bool SysInitCommon(void);

keycode_t MapGLFWKey(int key);
