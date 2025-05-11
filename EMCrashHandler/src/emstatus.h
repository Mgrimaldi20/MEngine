#pragma once

#include <stdbool.h>

#define EMTRACE_MAX_USERDATA_SIZE 1024
#define EMTRACE_MAX_FRAMES 100
#define EMTRACE_MAX_FRAME_SIZE 256

typedef enum
{
	EMSTATUS_EXIT_OK = 0,	// the engine has exited successfully and with no errors
	EMSTATUS_EXIT_ERROR		// the engine has exited with a known error
} emexitcode_t;

typedef struct
{
	bool connected;
	emexitcode_t status;
	unsigned char userdata[EMTRACE_MAX_USERDATA_SIZE];
	char stacktrace[EMTRACE_MAX_FRAMES][EMTRACE_MAX_FRAME_SIZE];
} emstatus_t;
