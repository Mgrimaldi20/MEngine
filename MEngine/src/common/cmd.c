#include <stdio.h>
#include <string.h>
#include "sys/sys.h"
#include "common.h"

#define DEF_CMD_MAP_CAPACITY 256
#define DEF_CMD_BUFFER_SIZE 0xffff

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
static char cmdbuffer[DEF_CMD_BUFFER_SIZE];
static size_t cmdbufferlen;

static bool initialized;

/*
* Function: HashCmdName
* Hashes the name of the command to generate an index, using the FNV-1a algorithm
* 
* 	name: The name of the command
* 
* Returns: The hash value
*/
static size_t HashCmdName(const char *name)
{
	size_t hash = 2166136261u;	// initial offset basis, large prime number
	size_t len = Sys_Strlen(name, CMD_MAX_STR_LEN);

	for (size_t i=0; i<len; i++)
	{
		hash ^= (unsigned char)name[i];
		hash *= 16777619;	// FNV prime number
	}

	return(hash & (cmdmap->capacity - 1));
}

/*
* Function: FindCommand
* Finds a command in the hashmap
* 
* 	name: The name of the command
* 
* Returns: The command if found, NULL otherwise
*/
static cmd_t *FindCommand(const char *name)
{
	if (!name || !name[0])
		return(NULL);

	size_t index = HashCmdName(name);
	cmdentry_t *current = cmdmap->cmds[index];
	while (current)
	{
		if (strcmp(current->value->name, name) == 0)
			return(current->value);

		current = current->next;
	}

	return(NULL);
}

/*
* Function: ExecuteCommand
* Tokenizes and executes a command
* 
* 	cmdstr: The command string to execute
*/
static void ExecuteCommand(const char *cmdstr)
{
	cmdargs_t args = { 0 };

	snprintf(args.cmdstr, CMD_MAX_STR_LEN, "%s", cmdstr);

	char *token = NULL, *saveptr = NULL;
	bool inquotes = false;
	char argbuffer[CMD_MAX_STR_LEN] = { 0 };
	size_t argbufferlen = 0;

	token = Sys_Strtok(args.cmdstr, " ", &saveptr);		// tokenize the command string into argc/argv style data
	while (token)
	{
		if (args.argc > CMD_MAX_ARGS)
		{
			Log_Writef(LOG_WARN, "Failed to execute command, too many arguments: %s", cmdstr);
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
			argbufferlen += Sys_Strlen(token, CMD_MAX_STR_LEN);

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
		Log_Writef(LOG_WARN, "Failed to execute command, command not found: %s", args.argv[0]);
		return;
	}

	cmd->function(&args);
}

/*
* Function: Help_Cmd
* The help command function, prints out the standard help message, or the description of a specific command
* 
* 	args: The command arguments, argv[1] is the command to get the description of
*/
static void Help_Cmd(const cmdargs_t *args)
{
	if ((args->argc < 2) || (args->argc > 2))
	{
		Common_Printf("Usage: %s | %s [command]", args->argv[0], args->argv[0]);
		return;
	}

	cmd_t *cmd = FindCommand(args->argv[1]);
	if (!cmd)
	{
		Common_Warnf("Failed to execute \"%s\" command, command not found: %s", args->argv[0], args->argv[1]);
		return;
	}

	Common_Printf("Command: %s, Description: %s", cmd->name, cmd->description);
}

/*
* Function: Cmd_Init
* Initializes the command system
* 
* Returns: A boolean if initialization was successful or not
*/
bool Cmd_Init(void)
{
	if (initialized)
		return(true);

	cmdmap = MemCache_Alloc(sizeof(*cmdmap));
	if (!cmdmap)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for command map");
		return(false);
	}

	cmdmap->capacity = DEF_CMD_MAP_CAPACITY;
	cmdmap->numcmds = 0;

	cmdmap->cmds = MemCache_Alloc(sizeof(*cmdmap->cmds) * cmdmap->capacity);
	if (!cmdmap->cmds)
	{
		Log_Write(LOG_ERROR, "Failed to allocate memory for command map entries");
		MemCache_Free(cmdmap);
		return(false);
	}

	for (size_t i=0; i<cmdmap->capacity; i++)
		cmdmap->cmds[i] = NULL;

	Cmd_RegisterCommand("help", Help_Cmd, "Prints out the help message or the description of a specific command");

	initialized = true;

	return(true);
}

/*
* Function: Cmd_Shutdown
* Shuts down the command system
*/
void Cmd_Shutdown(void)
{
	if (!initialized)
		return;

	Log_Write(LOG_INFO, "Shutting down command system");

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

/*
* Function: Cmd_RegisterCommand
* Registers a command with the command system and adds it to the hashmap
* 
* 	name: The name of the command
* 	function: The function to execute when the command is called
* 	description: The description of the command
*/
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
		Log_Writef(LOG_WARN, "Failed to register command, command already exists: %s", name);
		return;
	}

	cmd_t *cmd = MemCache_Alloc(sizeof(*cmd));
	if (!cmd)
	{
		Log_Writef(LOG_ERROR, "Failed to allocate memory for command: %s", name);
		return;
	}

	cmd->name = name;
	cmd->description = description;
	cmd->function = function;

	cmdentry_t *entry = MemCache_Alloc(sizeof(*entry));
	if (!entry)
	{
		Log_Writef(LOG_ERROR, "Failed to allocate memory for command entry: %s", name);
		MemCache_Free(cmd);
		return;
	}

	entry->value = cmd;
	entry->next = NULL;

	size_t index = HashCmdName(name);

	if (!cmdmap->cmds[index])
		cmdmap->cmds[index] = entry;

	else	// collision, add to start of list
	{
		entry->next = cmdmap->cmds[index];
		cmdmap->cmds[index] = entry;
	}

	cmdmap->numcmds++;

	if (cmdmap->numcmds >= (cmdmap->capacity * 0.75))	// if the number of cmds is at 75% capacity, resize the map to double
	{
		cmdmap->capacity *= 2;

		cmdentry_t **newcmds = MemCache_Alloc(sizeof(*newcmds) * cmdmap->capacity);
		if (!newcmds)
		{
			Log_Write(LOG_ERROR, "Failed to allocate memory for new command map");
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
				size_t newindex = HashCmdName(current->value->name);

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

/*
* Function: Cmd_RemoveCommand
* Removes a command from the command system and the hashmap
* 
* 	name: The name of the command
*/
void Cmd_RemoveCommand(const char *name)
{
	if (!name || !name[0])
	{
		Log_Write(LOG_WARN, "Failed to remove command, invalid command name: Command name could be empty");
		return;
	}

	size_t index = HashCmdName(name);

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

/*
* Function: Cmd_BufferCommand
* Buffers a command to be executed, appends the command to the buffer or executes it immediately depending on the execution type
* 
* 	exec: The execution type of the command, immediate or buffered
* 	cmd: The command string to buffer including arguments, already terminated with \n
*/
void Cmd_BufferCommand(const cmdexecution_t exec, const char *cmd)
{
	if (!cmd || !cmd[0])
	{
		Log_Write(LOG_WARN, "Failed to buffer command, invalid command string: Command string could be empty");
		return;
	}

	size_t len = Sys_Strlen(cmd, CMD_MAX_STR_LEN);
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
			Log_Writef(LOG_WARN, "Failed to buffer command, invalid command execution type: %d", exec);
			break;
	}
}

/*
* Function: Cmd_ExecuteCommandBuffer
* Tokenizes and executes the commands in the buffer
*/
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
