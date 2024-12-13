#include <stdio.h>
#include "common/common.h"

#include "posixlocal.h"

posixvars_t posixstate;

int main(int argc, char **argv)
{
	posixstate = (posixvars_t){ 0 };

	posixstate.argc = argc;
	posixstate.argv = argv;
	
	if (!Common_Init())
	{
		Common_Shutdown();
		return(1);
	}

	while (!glfwWindowShouldClose(posixstate.window))
	{
		Common_Frame();
		glfwPollEvents();
	}

	Common_Shutdown();
	
	return(0);
}
