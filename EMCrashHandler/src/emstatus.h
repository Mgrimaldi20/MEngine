#pragma once

#define EMTRACE_MAX_FRAMES 100
#define EMTRACE_MAX_FRAME_LEN 256
#define EMTRACE_MAX_USERDATA_SIZE 1024

typedef enum
{
	EMSTATUS_OK = 0,		// heartbeat signal, the engine is running fine
	EMSTATUS_EXIT_OK,		// exit code, the engine has exited successfully and with no errors
	EMSTATUS_EXIT_ERROR		// exit code, the engine has exited with a known error
} emstatuscode_t;

typedef struct
{
	emstatuscode_t status;
	char stacktrace[EMTRACE_MAX_FRAMES][EMTRACE_MAX_FRAME_LEN];
	unsigned char userdata[EMTRACE_MAX_USERDATA_SIZE];
} emstatus_t;
