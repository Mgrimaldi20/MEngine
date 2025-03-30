#include <stdio.h>
#include <time.h>
#include "emgl.h"
#include "common/common.h"
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
static cvar_t *rfov;

static bool initialized;

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
	{ "640x480", 640, 480 }
};

/*
* Function: GetVideoModeInfo
* Gets the width and height of the window based on the video mode, or custom mode if -1 is passed
* 
* 	width: A pointer to the width of the window
* 	height: A pointer to the height of the window
* 	mode: The video mode to use, -1 for custom width and height, otherwise the index of the predefined video mode, all modes are 16:9 aspect ratio
* 
* Returns: A boolean if the operation was successful or not
*/
static bool GetVideoModeInfo(int *width, int *height, int mode)
{
	int modesize = (sizeof(videomodes) / sizeof(videomodes[0]));
	if (mode < -1 || mode >= modesize)
		return(false);

	if (mode == -1)
	{
		Log_WriteSeq(LOG_WARN, "Using a custom video mode, might not render correctly if aspect ratio is non standard");

		int w = 0;
		if (!Cvar_GetInt(rwidth, &w))
		{
			Log_WriteSeq(LOG_WARN, "Failed to get r_width cvar, using the default width: %d", R_DEF_WIN_WIDTH);
			*width = R_DEF_WIN_WIDTH;
		}

		int h = 0;
		if (!Cvar_GetInt(rheight, &h))
		{
			Log_WriteSeq(LOG_WARN, "Failed to get r_height cvar, using the default height: %d", R_DEF_WIN_HEIGHT);
			*height = R_DEF_WIN_HEIGHT;
		}

		*width = w;
		*height = h;

		Log_WriteSeq(LOG_INFO, "Using custom video mode: %dx%d", *width, *height);

		return(true);
	}

	*width = videomodes[mode].width;
	*height = videomodes[mode].height;

	Log_WriteSeq(LOG_INFO, "Using video mode: %s", videomodes[mode].mode);

	return(true);
}

/*
* Function: GLDebugCallback
* The OpenGL debug callback function prototype, logging for OpenGL messages, only used in engine debug mode
*
* 	source: The source of the message, API, window system, etc...
* 	type: The type of message, error, warning, etc...
* 	id: The message ID
* 	severity: The severity of the message, high, medium, low, etc...
* 	length: The length of the message
* 	message: The message
* 	userparam: The user parameter, an object or pointer to user data passed to the callback, can be referenced in the callback
*/
static void GLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userparam)
{
	(const void *)userparam;
	(GLsizei)length;

	const char *sourcestr = NULL;
	const char *typestr = NULL;
	const char *severitystr = NULL;
	logtype_t logtype = LOG_INFO;

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:
			sourcestr = "Source: API";
			break;

		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			sourcestr = "Source: Window System";
			break;

		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			sourcestr = "Source: Shader Compiler";
			break;

		case GL_DEBUG_SOURCE_THIRD_PARTY:
			sourcestr = "Source: Third Party";
			break;

		case GL_DEBUG_SOURCE_APPLICATION:
			sourcestr = "Source: Application";
			break;

		case GL_DEBUG_SOURCE_OTHER:
			sourcestr = "Source: Other";
			break;

		default:
			sourcestr = "Source: Unknown";
			break;
	}

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:
			typestr = "Type: Error";
			break;

		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			typestr = "Type: Deprecated Behavior";
			break;

		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			typestr = "Type: Undefined Behavior";
			break;

		case GL_DEBUG_TYPE_PORTABILITY:
			typestr = "Type: Portability";
			break;

		case GL_DEBUG_TYPE_PERFORMANCE:
			typestr = "Type: Performance";
			break;

		case GL_DEBUG_TYPE_OTHER:
			typestr = "Type: Other";
			break;

		case GL_DEBUG_TYPE_MARKER:
			typestr = "Type: Marker";
			break;

		default:
			typestr = "Type: Unknown";
			break;
	}

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:
			severitystr = "Severity: High";
			logtype = LOG_ERROR;
			break;

		case GL_DEBUG_SEVERITY_MEDIUM:
			severitystr = "Severity: Medium";
			logtype = LOG_WARN;
			break;

		case GL_DEBUG_SEVERITY_LOW:
			severitystr = "Severity: Low";
			logtype = LOG_WARN;
			break;

		case GL_DEBUG_SEVERITY_NOTIFICATION:
			severitystr = "Severity: Notification";
			logtype = LOG_INFO;
			break;

		default:
			severitystr = "Severity: Unknown";
			logtype = LOG_INFO;
			break;
	}

	const char *logmsg = "OpenGL Debug Message: %s, %s, %d, %s, %s";

	switch (logtype)
	{
		case LOG_INFO:
			Common_Printf(logmsg, sourcestr, typestr, id, severitystr, message);
			break;

		case LOG_WARN:
			Common_Warnf(logmsg, sourcestr, typestr, id, severitystr, message);
			break;

		case LOG_ERROR:
			Common_Errorf(logmsg, sourcestr, typestr, id, severitystr, message);
			break;
	}
}


/*
* Function: InitOpenGL
* Initializes the OpenGL state and options
*/
static void InitOpenGL(void)
{
	// get the debug output info if the bit is set (engine is in debug mode)
	GLint flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(GLDebugCallback, NULL);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
	}

	glViewport(0, 0, (GLsizei)glstate.width, (GLsizei)glstate.height);

	glShadeModel(GL_SMOOTH);	// sets shading to smooth
	glClearIndex(0.0f);			// set BG to black
	glClearDepth(1.0);			// clear depth buffer
	glEnable(GL_DEPTH_TEST);	// enables Z-Buffering
	glDepthFunc(GL_LEQUAL);		// depth test calculation
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);	// better perspective correction
}

/*
* Function: Sizeviewport_Cmd
* Resizes the rendering viewport on window size change, this is just the client region
* 
*	args: The command arguments, this function takes no arguments
*/
static void Sizeviewport_Cmd(const cmdargs_t *args)
{
	if (args->argc != 1)
	{
		Common_Printf("%s : Invalid command usage, command takes no args", args->argv[0]);
		return;
	}

	glViewport(0, 0, (GLsizei)glstate.width, (GLsizei)glstate.height);
	Common_Printf("Resized viewport to %dx%d", glstate.width, glstate.height);
}

/*
* Function: Render_Init
* Initializes the rendering system and sets up the window
* 
* Returns: A boolean if the initialization was successful or not
*/
bool Render_Init(void)
{
	if (initialized)
		return(true);

	glstate = (glstate_t)
	{
		.width = 0,
		.height = 0,
		.fov = R_DEF_FOV
	};

	Cmd_RegisterCommand("sizeviewport", Sizeviewport_Cmd, "Resizes the viewport to the window size, happens on screen size change");

	rwidth = Cvar_RegisterInt("r_width", R_DEF_WIN_WIDTH, CVAR_ARCHIVE | CVAR_RENDERER, "Custom width of the window");
	rheight = Cvar_RegisterInt("r_height", R_DEF_WIN_HEIGHT, CVAR_ARCHIVE | CVAR_RENDERER, "Custom height of the window");
	rfullscreen = Cvar_RegisterBool("r_fullscreen", R_DEF_FULLSCREEN, CVAR_ARCHIVE | CVAR_RENDERER, "Fullscreen mode");
	rmultisamples = Cvar_RegisterInt("r_multisamples", R_DEF_MULTISAMPLES, CVAR_ARCHIVE | CVAR_RENDERER, "Multisample anti-aliasing");
	rrefresh = Cvar_RegisterInt("r_refresh", R_DEF_REFRESH_RATE, CVAR_ARCHIVE | CVAR_RENDERER, "Refresh rate of the monitor");
	rvsync = Cvar_RegisterInt("r_vsync", R_DEF_VSYNC, CVAR_ARCHIVE | CVAR_RENDERER, "Vertical sync");
	rfov = Cvar_RegisterFloat("r_fov", R_DEF_FOV, CVAR_ARCHIVE | CVAR_RENDERER, "Field of view");

	if (!GetVideoModeInfo(&glstate.width, &glstate.height, -1))
		return(false);

	bool fullscreen = false;
	if (!Cvar_GetBool(rfullscreen, &fullscreen))
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_fullscreen cvar, using the default value: %d", R_DEF_FULLSCREEN);
		fullscreen = R_DEF_FULLSCREEN;
	}

	int multisamples = 0;
	if (!Cvar_GetInt(rmultisamples, &multisamples))
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_multisamples cvar, using the default value: %d", R_DEF_MULTISAMPLES);
		multisamples = R_DEF_MULTISAMPLES;
	}

	int refreshrate = 0;
	if (!Cvar_GetInt(rrefresh, &refreshrate))
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_refresh cvar, using the default value: %d", R_DEF_REFRESH_RATE);
		refreshrate = R_DEF_REFRESH_RATE;
	}

	glwndparams_t params =
	{
		.fullscreen = fullscreen,
		.width = glstate.width,
		.height = glstate.height,
		.multisamples = multisamples,
		.refreshrate = refreshrate,
		.wndname = gameservices.gamename
	};

	if (!GLWnd_Init(params))	// create the window and get gl function pointers
		return(false);

	int vsync = 0;
	if (!Cvar_GetInt(rvsync, &vsync))
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_vsync cvar, using the default value: %d", R_DEF_VSYNC);
		vsync = R_DEF_VSYNC;
	}

	float fov = 0.0f;
	if (!Cvar_GetFloat(rfov, &fov))
	{
		Log_WriteSeq(LOG_WARN, "Failed to get r_fov cvar, using the default value: %f", R_DEF_FOV);
		fov = R_DEF_FOV;
	}

	glstate.fov = (double)fov;

	GLWnd_SetVSync(vsync);
	Log_WriteSeq(LOG_INFO, "Using VSync, value set: %ld", vsync);

	InitOpenGL();

	Log_WriteSeq(LOG_INFO, "OpenGL version: %s", (const char *)glGetString(GL_VERSION));

	initialized = true;
	glstate.initialized = initialized;

	return(true);
}

/*
* Function: Render_Shutdown
* Shuts down the rendering system
*/
void Render_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down rendering system");

	GLWnd_Shutdown();

	initialized = false;
	glstate.initialized = initialized;
}

/*
* Function: Render_GetMinWidth
* Gets the minimum width of the window, this is the smallest width a window can be
* 
* Returns: The minimum width of the window from the predefined video modes
*/
int Render_GetMinWidth(void)
{
	return(videomodes[sizeof(videomodes) / sizeof(videomodes[0]) - 1].width);
}

/*
* Function: Render_GetMinHeight
* Gets the minimum height of the window, this is the smallest height a window can be
* 
* Returns: The minimum height of the window from the predefined video modes
*/
int Render_GetMinHeight(void)
{
	return(videomodes[sizeof(videomodes) / sizeof(videomodes[0]) - 1].height);
}
