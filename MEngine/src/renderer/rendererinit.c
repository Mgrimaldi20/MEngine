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

static cvar_t *rwidth;
static cvar_t *rheight;
static cvar_t *rfullscreen;
static cvar_t *rmultisamples;
static cvar_t *rrefresh;
static cvar_t *rvsync;

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

static bool GetVideoModeInfo(int *width, int *height, int mode)
{
	int modesize = (sizeof(videomodes) / sizeof(videomodes[0]));
	if (mode < -1 || mode >= modesize)
		return(false);

	if (mode == -1)
	{
		Log_WriteSeq(LOG_WARN, "Using a custom video mode, might not render correctly if aspect ratio is non standard");

		int *w = CVar_GetInt(rwidth);
		if (!w)
		{
			Log_WriteSeq(LOG_WARN, "Failed to get r_width cvar, using the default width: %d", R_DEF_WIN_WIDTH);
			*width = R_DEF_WIN_WIDTH;
		}

		int *h = CVar_GetInt(rheight);
		if (!h)
		{
			Log_WriteSeq(LOG_WARN, "Failed to get r_height cvar, using the default height: %d", R_DEF_WIN_HEIGHT);
			*height = R_DEF_WIN_HEIGHT;
		}

		if (w)
			*width = *w;

		if (h)
			*height = *h;

		Log_WriteSeq(LOG_INFO, "Using custom video mode: %dx%d", *width, *height);

		return(true);
	}

	*width = videomodes[mode].width;
	*height = videomodes[mode].height;

	Log_WriteSeq(LOG_INFO, "Using video mode: %s", videomodes[mode].mode);

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
	glstate = (glstate_t)
	{
		.viewportsized = false,
		.width = 0,
		.height = 0,
		.fov = 60.0
	};

	rwidth = CVar_RegisterInt("r_width", R_DEF_WIN_WIDTH, CVAR_INT, CVAR_ARCHIVE | CVAR_RENDERER, "Custom width of the window");
	rheight = CVar_RegisterInt("r_height", R_DEF_WIN_HEIGHT, CVAR_INT, CVAR_ARCHIVE | CVAR_RENDERER, "Custom height of the window");
	rfullscreen = CVar_RegisterBool("r_fullscreen", R_DEF_FULLSCREEN, CVAR_BOOL, CVAR_ARCHIVE | CVAR_RENDERER, "Fullscreen mode");
	rmultisamples = CVar_RegisterInt("r_multisamples", R_DEF_MULTISAMPLES, CVAR_INT, CVAR_ARCHIVE | CVAR_RENDERER, "Multisample anti-aliasing");
	rrefresh = CVar_RegisterInt("r_refresh", R_DEF_REFRESH_RATE, CVAR_INT, CVAR_ARCHIVE | CVAR_RENDERER, "Refresh rate of the monitor");
	rvsync = CVar_RegisterInt("r_vsync", R_DEF_VSYNC, CVAR_INT, CVAR_ARCHIVE | CVAR_RENDERER, "Vertical sync");

	if (!GetVideoModeInfo(&glstate.width, &glstate.height, -1))
		return(false);

	bool fullscreen = false;
	bool *fs = CVar_GetBool(rfullscreen);
	if (!fs)
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_fullscreen cvar, using the default value: %d", R_DEF_FULLSCREEN);
		fullscreen = R_DEF_FULLSCREEN;
	}

	else
		fullscreen = *fs;

	int multisamples = 0;
	int *ms = CVar_GetInt(rmultisamples);
	if (!ms)
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_multisamples cvar, using the default value: %d", R_DEF_MULTISAMPLES);
		multisamples = R_DEF_MULTISAMPLES;
	}

	else
		multisamples = *ms;

	int refreshrate = 0;
	int *rr = CVar_GetInt(rrefresh);
	if (!rr)
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_refresh cvar, using the default value: %d", R_DEF_REFRESH_RATE);
		refreshrate = R_DEF_REFRESH_RATE;
	}

	else
		refreshrate = *rr;

	glwndparams_t params =
	{
		.fullscreen = fullscreen,
		.width = glstate.width,
		.height = glstate.height,
		.multisamples = multisamples,
		.refreshrate = refreshrate,
		.wndname = gameservices.gamename
	};

	if (!GLWnd_Init(params))	// create the window
		return(false);

	int vsync = 0;
	int *vs = CVar_GetInt(rvsync);
	if (!vs)
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_vsync cvar, using the default value: %d", 0);
		vsync = 0;
	}

	else
		vsync = *vs;

	GLWnd_SetVSync(vsync);
	Log_WriteSeq(LOG_INFO, "Using VSync, value set: %ld", vsync);

	InitOpenGL();

	Log_WriteSeq(LOG_INFO, "OpenGL version: %s", (const char *)glGetString(GL_VERSION));

	return(true);
}

void Render_Shutdown(void)
{
	GLWnd_Shutdown();
}
