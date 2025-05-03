#pragma once

#define EMTRACE_MAX_FRAMES 100
#define EMTRACE_MAX_FRAME_LEN 256
#define EMTRACE_MAX_USERDATA_SIZE 1024

typedef enum
{
	EMSTATUS_OK = 0,
	EMSTATUS_ERROR = 1
} emstatuscode_t;

typedef struct
{
	emstatuscode_t status;
	char stacktrace[EMTRACE_MAX_FRAMES][EMTRACE_MAX_FRAME_LEN];
	unsigned char userdata[EMTRACE_MAX_USERDATA_SIZE];
} emstatus_t;
