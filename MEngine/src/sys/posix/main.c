#include <stdio.h>
#include "common/common.h"

#include "posixlocal.h"

posixvars_t posixstate;

int main(int argc, char **argv)
{
	posixstate = (posixvars_t) { 0 };

	posixstate.argc = argc;
	posixstate.argv = argv;

	const rlim_t stacksize = 10 * 1024 * 1024;	// 10MB stack size
	struct rlimit rl;
	int result;

	result = getrlimit(RLIMIT_STACK, &rl);
	if (result == 0)
	{
		if (rl.rlim_cur < stacksize)
		{
			rl.rlim_cur = stacksize;
			result = setrlimit(RLIMIT_STACK, &rl);
			if (result != 0)
				Common_Errorf("Failed to set stack size");
		}
	}

	else
		Common_Errorf("Failed to get stack size");
	
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
