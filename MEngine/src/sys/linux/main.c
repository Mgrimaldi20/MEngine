#include <stdio.h>
#include "../../common/common.h"

#include "linuxlocal.h"

linuxvars_t linuxstate;

int main(int argc, char **argv)
{
	linuxstate = (linuxvars_t){ 0 };

	linuxstate.argc = argc;
	linuxstate.argv = argv;

	cmdline[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS];
	ProcessCommandLine(argc, argv, cmdline);
	
	if (!Common_Init(cmdline))
	{
		Common_Shutdown();
		return(1);
	}

	Log_WriteSeq(LOG_INFO, "Engine Initialized successfully...");

	if (Common_HelpMode())
	{
		Common_PrintHelpMsg();
		Common_Shutdown();
		return(0);
	}
	
	return(0);
}
