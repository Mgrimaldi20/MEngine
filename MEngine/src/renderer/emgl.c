#include "renderer/emgl.h"
#include "common/common.h"
#include "sys/sys.h"

static void *gldllhandle;

static bool initialized;

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
	if (initialized)
		return(true);

	if (gldllhandle)
		return(true);

	gldllhandle = Sys_LoadDLL(dllname);
	if (!gldllhandle)
	{
		Log_Write(LOG_INFO, "Failed to load OpenGL library: %s", dllname);
		return(false);
	}

	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)Sys_GetProcAddress(gldllhandle, "glDebugMessageCallback");
	glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)Sys_GetProcAddress(gldllhandle, "glDebugMessageControl");

	initialized = true;

	return(true);
}

/*
* Function: EMGL_Shutdown
* Unloads the OpenGL library, and removes the function pointers
*/
void EMGL_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down OpenGL library");

	if (gldllhandle)
	{
		Sys_UnloadDLL(gldllhandle);
		gldllhandle = NULL;
	}

	initialized = false;
}
