#include <string.h>
#include "common/common.h"
#include "sys/sys.h"
#include "posixlocal.h"

/*
* Function: Sys_Init
* Initializes the system services and gets the OS system information for Linux systems
* 
* Returns: A boolean if the initialization was successful or not
*/
bool Sys_Init(void)
{
	if (posixstate.initialized)
		return(true);

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
			"This code has only been tested on Ubuntu 20.4 "
			"but most the common and shared code should work the same, use with caution, "
			"some of the system frameworks might not work as expected");

	else if (strcmp(posixstate.osinfo.sysname, "Linux") != 0)
	{
		Sys_Error("Requires a Linux OS to run, you are using: %s", posixstate.osinfo.sysname);
		return(false);
	}

	Log_WriteSeq(LOG_INFO, "System memory: %lluMB", Sys_GetSystemMemory());
	Log_WriteSeq(LOG_INFO, "System supports max threads: %lu", Sys_GetMaxThreads());

	CVar_RegisterString("g_gamedll", "DemoGame.so", CVAR_GAME, "The name of the game DLL for Linux systems");
	CVar_RegisterString("g_demogamedll", "DemoGame.so", CVAR_GAME | CVAR_READONLY, "The name of the demo game DLL for Linux systems");

	posixstate.initialized = true;

	return(true);
}
