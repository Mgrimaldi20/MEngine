#pragma once

#include <stdbool.h>

#define EMCH_MAX_USERDATA_SIZE 1024
#define EMCH_MAX_FRAMES 100
#define EMCH_MAX_FRAME_SIZE 256

typedef enum
{
	EMSTATUS_EXIT_OK = 0,	// the engine has exited successfully and with no errors
	EMSTATUS_EXIT_ERROR		// the engine has exited with a known error
} emexitcode_t;

typedef struct
{
	bool connected;
	emexitcode_t status;
	unsigned char userdata[EMCH_MAX_USERDATA_SIZE];
	char stacktrace[EMCH_MAX_FRAMES][EMCH_MAX_FRAME_SIZE];
} emstatus_t;
