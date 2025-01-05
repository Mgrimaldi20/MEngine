#include "common.h"

#define DEF_CMD_MAP_CAPACITY 256
#define DEF_CMD_BUFFER_SIZE 1024

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
static char *cmdbuffer;

static bool initialized;

bool Cmd_Init(void)
{
	if (initialized)
		return(true);

	cmdbuffer = MemCache_Alloc(sizeof(*cmdbuffer) * DEF_CMD_BUFFER_SIZE);
	if (!cmdbuffer)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command buffer");
		return(false);
	}

	cmdmap = MemCache_Alloc(sizeof(*cmdmap));
	if (!cmdmap)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command map");
		MemCache_Free(cmdbuffer);
		return(false);
	}

	cmdmap->capacity = DEF_CMD_MAP_CAPACITY;
	cmdmap->numcmds = 0;

	cmdmap->cmds = MemCache_Alloc(sizeof(*cmdmap->cmds) * cmdmap->capacity);
	if (!cmdmap->cmds)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command map entries");
		MemCache_Free(cmdmap);
		MemCache_Free(cmdbuffer);
		return(false);
	}

	for (size_t i=0; i<cmdmap->capacity; i++)
		cmdmap->cmds[i] = NULL;

	initialized = true;

	return(true);
}

void Cmd_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down command system");

	for (size_t i=0; i<cmdmap->capacity; i++)
	{
		cmdentry_t *current = cmdmap->cmds[i];
		while (current)
		{
			cmdentry_t *next = current->next;
			MemCache_Free(current->value);
			MemCache_Free(current);
			current = next;
		}
	}

	MemCache_Free(cmdmap->cmds);
	MemCache_Free(cmdmap);

	MemCache_Free(cmdbuffer);

	initialized = false;
}

void Cmd_RegisterCommand(const char *name, const char *description, cmdfunction_t function)
{
}

void Cmd_RemoveCommand(const char *name)
{
}

void Cmd_BufferCommand(const cmdexecution_t exec, const char *cmd)
{
}
