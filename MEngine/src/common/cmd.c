#include "common.h"

#define DEF_CMD_MAP_CAPACITY 256

typedef struct
{
	const char *name;
	const char *description;
	cmdfunction_t function;
} cmd_t;

typedef struct cmdentry
{
	cmd_t *value;
	struct cmdentry *next;
} cmdentry_t;

typedef struct
{
	size_t numcmds;
	size_t capacity;
	cmdentry_t **cmds;
} cmdmap_t;

static cmdmap_t *cmdmap;

bool Cmd_Init(void)
{
	return(true);
}

void Cmd_Shutdown(void)
{
}

void Cmd_AddCommand(const char *name, const char *description, cmdfunction_t function)
{
}
