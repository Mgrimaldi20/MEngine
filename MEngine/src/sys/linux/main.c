#include <stdio.h>
#include "../../common/common.h"
#include "linuxlocal.h"

int main(int argc, char **argv)
{
	cmdline[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS];
	ProcessCommandLine(argc, argv, cmdline);
	
	if (!Common_Init(cmdline))
	{
		Common_Shutdown();
		ShutdownConsole();
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

void ProcessCommandLine(int argc, char **argv, char cmdout[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS])
{
	char *saveptr = NULL;
	int i = 0;

	for (int j=1; j<argc && i<MAX_CMDLINE_ARGS; j++)
	{
		char *token = Sys_Strtok(argv[j], " -", &saveptr);
		while (token != NULL)
		{
			snprintf(cmdout[i], MAX_CMDLINE_ARGS, "%s", token);
			token = Sys_Strtok(NULL, " -", &saveptr);
			i++;
		}
	}
}
