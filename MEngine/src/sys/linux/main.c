#include <stdio.h>
#include "common/common.h"

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

	while (!linuxstate.windowtoclose)
	{
		Common_Frame();
		glfwPollEvents();
	}

	Common_Shutdown();
	
	return(0);
}
