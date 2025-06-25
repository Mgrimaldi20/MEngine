#include "emgl.h"
#include "renderer.h"
#include "common/common.h"
#include "sys/sys.h"

static void *dllhandle;

static prochandle_t emgldebugmessagecallback;
static prochandle_t emgldebugmessagecontrol;

PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback;
PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl;

/*
* Function: EMGL_Init
* Loads the OpenGL library, assigns the function pointers
* 
* 	dllname: The name of the OpenGL library to load
* 
* Returns: A boolean if the initialization was successful or not
*/
bool EMGL_Init(const char *dllname)
{
	if (dllhandle)
		return(true);

	Log_Writef(LOG_INFO, "Loading OpenGL library: %s", dllname);

	dllhandle = Sys_LoadDLL(dllname);
	if (!dllhandle)
	{
		Log_Writef(LOG_INFO, "Failed to load OpenGL library: %s", dllname);
		return(false);
	}

	emgldebugmessagecallback.obj = GLWnd_GetProcAddressGL(dllhandle, "glDebugMessageCallback");
	emgldebugmessagecontrol.obj = GLWnd_GetProcAddressGL(dllhandle, "glDebugMessageControl");

	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)emgldebugmessagecallback.func;
	glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)emgldebugmessagecontrol.func;

	return(true);
}

/*
* Function: EMGL_Shutdown
* Unloads the OpenGL library, and removes the function pointers
*/
void EMGL_Shutdown(void)
{
	Log_Writef(LOG_INFO, "Shutting down OpenGL library");

	if (dllhandle)
	{
		Sys_UnloadDLL(dllhandle);
		dllhandle = NULL;

		glDebugMessageCallback = NULL;
		glDebugMessageControl = NULL;
	}
}
