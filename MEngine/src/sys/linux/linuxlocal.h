#pragma once

#if !defined(__linux__) || !defined(__APPLE__)
#error Include is for Linux and Apple only
#endif

void ProcessCommandLine(int argc, char **argv, char cmdout[MAX_CMDLINE_ARGS][MAX_CMDLINE_ARGS]);

void PrintHelpMsg(void);
