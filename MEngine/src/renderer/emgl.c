#include "renderer/emgl.h"
#include "common/common.h"
#include "sys/sys.h"

static void *gldllhandle;

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
	if (gldllhandle)
		return(true);

	gldllhandle = Sys_LoadDLL(dllname);
	if (!gldllhandle)
	{
		Log_Write(LOG_INFO, "Failed to load OpenGL library: %s", dllname);
		return(false);
	}

	glDebugMessageCallback = (PFNGLDEBUGMESSAGECALLBACKPROC)wglGetProcAddress("glDebugMessageCallback");
	glDebugMessageControl = (PFNGLDEBUGMESSAGECONTROLPROC)wglGetProcAddress("glDebugMessageControl");

	return(true);
}

/*
* Function: EMGL_Shutdown
* Unloads the OpenGL library, and removes the function pointers
*/
void EMGL_Shutdown(void)
{
	Log_Write(LOG_INFO, "Shutting down OpenGL library");

	if (gldllhandle)
	{
		Sys_UnloadDLL(gldllhandle);
		gldllhandle = NULL;
	}

	glDebugMessageCallback = NULL;
	glDebugMessageControl = NULL;
}
