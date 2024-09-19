#include <stdio.h>
#include <time.h>
#include "../common/common.h"
#include "renderer.h"

typedef struct
{
	const char *mode;
	int width;
	int height;
} videomode_t;

static const videomode_t videomodes[] =
{
	// all modes are to be 16:9 aspect ratio
	{ "7680x4320", 7680, 4320 },
	{ "3840x2160", 3840, 2160 },
	{ "2560x1440", 2560, 1440 },
	{ "1920x1080", 1920, 1080 },
	{ "1600x900", 1600, 900 },
	{ "1366x768", 1366, 768 },
	{ "1280x720", 1280, 720 },
	{ "1024x576", 1024, 576 },
	{ "960x540", 960, 540 },
	{ "854x480", 854, 480 },
	{ "640x360", 640, 360 },
	{ "640x480", 640, 480 },
	{ "320x240", 320, 240 }
};

static bool GetVideoModeInfo(long *width, long *height, int mode)
{
	int modesize = (sizeof(videomodes) / sizeof(videomodes[0]));
	if (mode < -1 || mode >= modesize)
		return(false);

	if (mode == -1)
	{
		*width = Cfg_GetNum(menginecfg, "rWidth");
		*height = Cfg_GetNum(menginecfg, "rHeight");
		return(true);
	}

	*width = videomodes[mode].width;
	*height = videomodes[mode].height;

	return(true);
}

static void InitOpenGL(void)
{
	glViewport(0, 0, (GLsizei)glstate.width, (GLsizei)glstate.height);

	glShadeModel(GL_SMOOTH);	// sets shading to smooth
	glClearIndex(0.0f);			// set BG to black
	glClearDepth(1.0);			// clear depth buffer
	glEnable(GL_DEPTH_TEST);	// enables Z-Buffering
	glDepthFunc(GL_LEQUAL);		// depth test calculation
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// better perspective correction
}

bool Render_Init(void)
{
	if (!GetVideoModeInfo(&(long)glstate.width, &(long)glstate.height, -1))
		return(false);

	glwndparams_t params =
	{
		.fullscreen = (bool)Cfg_GetNum(menginecfg, "rFullscreen"),
		.width = glstate.width,
		.height = glstate.height,
		.multisamples = (long)Cfg_GetNum(menginecfg, "rMultiSamples"),
		.refreshrate = (long)Cfg_GetNum(menginecfg, "rRefresh"),
		.wndname = gameservices.gamename
	};

	params.fullscreen = false;	// TODO: just for testing, remove later in production

	if (!GLWnd_Init(params))	// create the window
		return(false);

	long vsync = (long)Cfg_GetNum(menginecfg, "rVsync");
	GLWnd_SetVSync((int)vsync);
	Log_WriteSeq(LOG_INFO, "Using VSync, value set: %ld", vsync);

	InitOpenGL();

	Log_WriteSeq(LOG_INFO, "OpenGL version: %s", (const char *)glGetString(GL_VERSION));

	return(true);
}

void Render_Shutdown(void)
{
	GLWnd_Shutdown();
}
