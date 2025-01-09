#include <stdio.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"

#define DEF_CMD_MAP_CAPACITY 256
#define DEF_CMD_BUFFER_SIZE CMD_MAX_STR_LEN

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
static char cmdbuffer[DEF_CMD_BUFFER_SIZE];		// TODO: might make this dynamic in the future again, just make it fixed len for now
static size_t cmdbufferlen;

static bool initialized;

static size_t HashFunction(const char *name)	// hash the name, same as the cvar hash function
{
	size_t hash = 0;
	size_t len = strnlen(name, CVAR_MAX_STR_LEN);

	for (size_t i=0; i<len; i++)
		hash = (hash * 31) + name[i];

	return(hash % cmdmap->capacity);
}

static cmd_t *FindCommand(const char *name)
{
	if (!name || !name[0])
		return(NULL);

	size_t index = HashFunction(name);
	cmdentry_t *current = cmdmap->cmds[index];
	while (current)
	{
		if (strcmp(current->value->name, name) == 0)
			return(current->value);

		current = current->next;
	}

	return(NULL);
}

static void ExecuteCommand(const char *cmdstr)	// tokenizes and executes
{
	cmdargs_t args = { 0 };

	snprintf(args.cmdstr, CMD_MAX_STR_LEN, "%s", cmdstr);

	char *token = NULL;
	char *saveptr = NULL;
	bool inquotes = false;
	char argbuffer[CMD_MAX_STR_LEN] = { 0 };
	size_t argbufferlen = 0;

	token = Sys_Strtok(args.cmdstr, " ", &saveptr);		// tokenize the command string into argc/argv style data
	while (token)
	{
		if (args.argc > CMD_MAX_ARGS)
		{
			Log_Write(LOG_WARN, "Failed to execute command, too many arguments: %s", cmdstr);
			return;
		}

		if (token[0] == '"')
		{
			inquotes = true;
			token++;
			argbuffer[0] = '\0';
			argbufferlen = 0;
		}

		if (inquotes)
		{
			char *endquote = strchr(token, '"');
			if (endquote)
			{
				*endquote = '\0';
				inquotes = false;
			}

			if (argbufferlen > 0)
			{
				strncat(argbuffer, " ", CMD_MAX_STR_LEN - argbufferlen - 1);
				argbufferlen++;
			}

			strncat(argbuffer, token, CMD_MAX_STR_LEN - argbufferlen - 1);
			argbufferlen += strlen(token);

			if (!inquotes)
			{
				args.argv[args.argc] = argbuffer;
				args.argc++;
			}
		}

		else
		{
			args.argv[args.argc] = token;
			args.argc++;
		}

		token = Sys_Strtok(NULL, " ", &saveptr);
	}

	if (args.argc == 0)
		return;

	cmd_t *cmd = FindCommand(args.argv[0]);
	if (!cmd)
	{
		Log_Write(LOG_WARN, "Failed to execute command, command not found: %s", args.argv[0]);
		return;
	}

	cmd->function(&args);
}

bool Cmd_Init(void)
{
	if (initialized)
		return(true);

	cmdmap = MemCache_Alloc(sizeof(*cmdmap));
	if (!cmdmap)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command map");
		return(false);
	}

	cmdmap->capacity = DEF_CMD_MAP_CAPACITY;
	cmdmap->numcmds = 0;

	cmdmap->cmds = MemCache_Alloc(sizeof(*cmdmap->cmds) * cmdmap->capacity);
	if (!cmdmap->cmds)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command map entries");
		MemCache_Free(cmdmap);
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

	initialized = false;
}

void Cmd_RegisterCommand(const char *name, cmdfunction_t function, const char *description)
{
	if (!name || !name[0])
	{
		Log_Write(LOG_WARN, "Failed to register command, invalid command name: Command name could be empty");
		return;
	}

	if (!function)
	{
		Log_Write(LOG_WARN, "Failed to register command, invalid command function: Command function could be NULL");
		return;
	}

	if (FindCommand(name))		// if the command already exists, dont register it
	{
		Log_WriteSeq(LOG_WARN, "Failed to register command, command already exists: %s", name);
		return;
	}

	cmd_t *cmd = MemCache_Alloc(sizeof(*cmd));
	if (!cmd)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command: %s", name);
		return;
	}

	cmd->name = name;
	cmd->description = description;
	cmd->function = function;

	cmdentry_t *entry = MemCache_Alloc(sizeof(*entry));
	if (!entry)
	{
		Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for command entry: %s", name);
		MemCache_Free(cmd);
		return;
	}

	entry->value = cmd;
	entry->next = NULL;

	size_t index = HashFunction(name);

	if (!cmdmap->cmds[index])
		cmdmap->cmds[index] = entry;

	else	// collision, add to start of list
	{
		entry->next = cmdmap->cmds[index];
		cmdmap->cmds[index] = entry;
	}

	cmdmap->numcmds++;

	// if the number of cvars is st 75% capacity, resize the map to double, move all the cvars to the new map, and free the old map
	if (cmdmap->numcmds >= (cmdmap->capacity * 0.75))
	{
		cmdmap->capacity *= 2;

		cmdentry_t **newcmds = MemCache_Alloc(sizeof(*newcmds) * cmdmap->capacity);
		if (!newcmds)
		{
			Log_WriteSeq(LOG_ERROR, "Failed to allocate memory for new command map");
			MemCache_Free(cmdmap->cmds);
			MemCache_Free(cmdmap);
			return;
		}

		for (size_t i=0; i<cmdmap->capacity; i++)
			newcmds[i] = NULL;

		for (size_t i=0; i<cmdmap->capacity/2; i++)
		{
			cmdentry_t *current = cmdmap->cmds[i];
			while (current)
			{
				cmdentry_t *next = current->next;
				size_t newindex = HashFunction(current->value->name);

				if (!newcmds[newindex])
					newcmds[newindex] = current;

				else
				{
					current->next = newcmds[newindex];
					newcmds[newindex] = current;
				}

				current = next;
			}
		}

		MemCache_Free(cmdmap->cmds);
		cmdmap->cmds = newcmds;
	}
}

void Cmd_RemoveCommand(const char *name)
{
	if (!name || !name[0])
	{
		Log_Write(LOG_WARN, "Failed to remove command, invalid command name: Command name could be empty");
		return;
	}

	size_t index = HashFunction(name);

	cmdentry_t *prev = NULL;
	cmdentry_t *current = cmdmap->cmds[index];
	while (current)
	{
		if (strcmp(current->value->name, name) == 0)
		{
			if (!prev)
				cmdmap->cmds[index] = current->next;

			else
				prev->next = current->next;

			MemCache_Free(current->value);
			MemCache_Free(current);
			cmdmap->numcmds--;
			return;
		}

		prev = current;
		current = current->next;
	}
}

void Cmd_BufferCommand(const cmdexecution_t exec, const char *cmd)
{
	if (!cmd || !cmd[0])
	{
		Log_Write(LOG_WARN, "Failed to buffer command, invalid command string: Command string could be empty");
		return;
	}

	size_t len = strnlen(cmd, CMD_MAX_STR_LEN);
	if ((len + cmdbufferlen) >= DEF_CMD_BUFFER_SIZE)
	{
		Log_Write(LOG_WARN, "Failed to buffer command, command buffer overflow");
		return;
	}

	switch (exec)
	{
		case CMD_EXEC_NOW:
			ExecuteCommand(cmd);
			break;

		case CMD_EXEC_APPEND:
			memcpy(cmdbuffer + cmdbufferlen, cmd, len);
			cmdbufferlen += len;
			cmdbuffer[cmdbufferlen] = '\n';
			cmdbufferlen++;
			break;

		default:
			Log_Write(LOG_ERROR, "Failed to buffer command, invalid command execution type: %d", exec);
			break;
	}
}

void Cmd_ExecuteCommandBuffer(void)
{
	char *cmd = cmdbuffer;
	while (cmdbufferlen > 0)
	{
		char *end = strchr(cmd, '\n');
		if (!end)
			end = cmd + cmdbufferlen;

		else
		{
			*end = '\0';
			end++;
		}

		ExecuteCommand(cmd);

		cmdbufferlen -= (end - cmd);
		cmd = end;
	}
}
