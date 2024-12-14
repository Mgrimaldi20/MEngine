#include <string.h>
#include "common/common.h"
#include "sys/sys.h"
#include "posixlocal.h"

bool Sys_Init(void)
{
	if (uname(&posixstate.osinfo) == -1) 	// get the OS version information, only run if the OS version is high enough
	{
		Log_WriteSeq(LOG_ERROR, "Failed to get OS version information");
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "OS: %s %s %s %s",
		posixstate.osinfo.sysname,
		posixstate.osinfo.nodename,
		posixstate.osinfo.release,
		posixstate.osinfo.version
	);

	if (Common_IgnoreOSVer())
		Log_WriteSeq(LOG_WARN, "Ignoring OS version check... "
			"This code has only been tested on MacOS Darwin Kernel Version 24.1.0 "
			"but most the common and shared code should work the same, use with caution, "
			"some of the system frameworks might not work as expected");

	else if (strcmp(posixstate.osinfo.sysname, "Darwin") != 0)
	{
		Sys_Error("Requires MacOS to run, you are using: %s", posixstate.osinfo.sysname);
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());

	CVar_RegisterString("g_gamedll", "DemoGame.dylib", CVAR_GAME, "The name of the game DLL for MacOS systems");
	CVar_RegisterString("g_demogamedll", "DemoGame.dylib", CVAR_GAME | CVAR_READONLY, "The name of the demo game DLL for MacOS systems");

	posixstate.initialized = true;

	return(true);
}
