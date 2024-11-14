#include <stdio.h>
#include "../../common/common.h"

#include "linuxlocal.h"

linuxvars_t linuxstate;

int main(int argc, char **argv)
{
	linuxstate = (linuxvars_t){ 0 };

	linuxstate.argc = argc;
	linuxstate.argv = argv;
	
	if (!Common_Init())
	{
		Common_Shutdown();
		return(1);
	}

	Log_WriteSeq(LOG_INFO, "Engine Initialized successfully...");

	while (!glfwWindowShouldClose(linuxstate.window))
	{
		Common_Frame();
		glfwPollEvents();
	}

	Log_WriteSeq(LOG_INFO, "Engine shutting down...");
	Common_Shutdown();
	
	return(0);
}
