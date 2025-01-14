#include <stdio.h>
#include <string.h>
#include "keycodes.h"
#include "sys/sys.h"
#include "common.h"

typedef struct
{
	const char *name;
	keycode_t key;
} keyname_t;

typedef struct
{
	bool down;
	unsigned int repeats;
	char *binding;
} key_t;

static const keyname_t keynames[] =
{
	{ "UNKNOWN", KEY_UNKNOWN },
	{ "CANCEL", KEY_CANCEL },
	{ "CLEAR", KEY_CLEAR },
	{ "SELECT", KEY_SELECT },
	{ "PRINT", KEY_PRINT },
	{ "EXEC", KEY_EXEC },
	{ "HELP", KEY_HELP },
	{ "APPLICATION", KEY_APPLICATION },
	{ "SLEEP", KEY_SLEEP },
	{ "SPACE", KEY_SPACE },
	{ "BACKSPACE", KEY_BACKSPACE },
	{ "DELETE", KEY_DELETE },
	{ "INSERT", KEY_INSERT },
	{ "HOME", KEY_HOME },
	{ "END", KEY_END },
	{ "PAGEUP", KEY_PAGEUP },
	{ "PAGEDOWN", KEY_PAGEDOWN },
	{ "TAB", KEY_TAB },
	{ "ENTER", KEY_ENTER },
	{ "ESCAPE", KEY_ESCAPE },
	{ "CAPSLOCK", KEY_CAPSLOCK },
	{ "PAUSE", KEY_PAUSE },
	{ "PRINTSCREEN", KEY_PRINTSCREEN },
	{ "SCROLLLOCK", KEY_SCROLLLOCK },
	{ "NUMLOCK", KEY_NUMLOCK },
	{ "CONTROL", KEY_CONTROL },
	{ "LEFTCONTROL", KEY_LEFTCONTROL },
	{ "RIGHTCONTROL", KEY_RIGHTCONTROL },
	{ "ALT", KEY_ALT },
	{ "LEFTALT", KEY_LEFTALT },
	{ "RIGHTALT", KEY_RIGHTALT },
	{ "SHIFT", KEY_SHIFT },
	{ "LEFTSHIFT", KEY_LEFTSHIFT },
	{ "RIGHTSHIFT", KEY_RIGHTSHIFT },
	{ "SPECIAL", KEY_SPECIAL },
	{ "LEFTSPECIAL", KEY_LEFTSPECIAL },
	{ "RIGHTSPECIAL", KEY_RIGHTSPECIAL },
	{ "UP", KEY_UP },
	{ "DOWN", KEY_DOWN },
	{ "LEFT", KEY_LEFT },
	{ "RIGHT", KEY_RIGHT },
	{ "0", KEY_0 },
	{ "1", KEY_1 },
	{ "2", KEY_2 },
	{ "3", KEY_3 },
	{ "4", KEY_4 },
	{ "5", KEY_5 },
	{ "6", KEY_6 },
	{ "7", KEY_7 },
	{ "8", KEY_8 },
	{ "9", KEY_9 },
	{ "A", KEY_A },
	{ "B", KEY_B },
	{ "C", KEY_C },
	{ "D", KEY_D },
	{ "E", KEY_E },
	{ "F", KEY_F },
	{ "G", KEY_G },
	{ "H", KEY_H },
	{ "I", KEY_I },
	{ "J", KEY_J },
	{ "K", KEY_K },
	{ "L", KEY_L },
	{ "M", KEY_M },
	{ "N", KEY_N },
	{ "O", KEY_O },
	{ "P", KEY_P },
	{ "Q", KEY_Q },
	{ "R", KEY_R },
	{ "S", KEY_S },
	{ "T", KEY_T },
	{ "U", KEY_U },
	{ "V", KEY_V },
	{ "W", KEY_W },
	{ "X", KEY_X },
	{ "Y", KEY_Y },
	{ "Z", KEY_Z },
	{ "F1", KEY_F1 },
	{ "F2", KEY_F2 },
	{ "F3", KEY_F3 },
	{ "F4", KEY_F4 },
	{ "F5", KEY_F5 },
	{ "F6", KEY_F6 },
	{ "F7", KEY_F7 },
	{ "F8", KEY_F8 },
	{ "F9", KEY_F9 },
	{ "F10", KEY_F10 },
	{ "F11", KEY_F11 },
	{ "F12", KEY_F12 },
	{ "F13", KEY_F13 },
	{ "F14", KEY_F14 },
	{ "F15", KEY_F15 },
	{ "F16", KEY_F16 },
	{ "F17", KEY_F17 },
	{ "F18", KEY_F18 },
	{ "F19", KEY_F19 },
	{ "F20", KEY_F20 },
	{ "F21", KEY_F21 },
	{ "F22", KEY_F22 },
	{ "F23", KEY_F23 },
	{ "F24", KEY_F24 },
	{ "F25", KEY_F25 },
	{ "NUMPAD0", KEY_NUMPAD0 },
	{ "NUMPAD1", KEY_NUMPAD1 },
	{ "NUMPAD2", KEY_NUMPAD2 },
	{ "NUMPAD3", KEY_NUMPAD3 },
	{ "NUMPAD4", KEY_NUMPAD4 },
	{ "NUMPAD5", KEY_NUMPAD5 },
	{ "NUMPAD6", KEY_NUMPAD6 },
	{ "NUMPAD7", KEY_NUMPAD7 },
	{ "NUMPAD8", KEY_NUMPAD8 },
	{ "NUMPAD9", KEY_NUMPAD9 },
	{ "MULTIPLY", KEY_MULTIPLY },
	{ "ADD", KEY_ADD },
	{ "SEPARATOR", KEY_SEPARATOR },
	{ "SUBTRACT", KEY_SUBTRACT },
	{ "DECIMAL", KEY_DECIMAL },
	{ "DIVIDE", KEY_DIVIDE },
	{ "SEMICOLON", KEY_SEMICOLON },
	{ "SLASH", KEY_SLASH },
	{ "GRAVE", KEY_GRAVE },
	{ "LEFTBRACKET", KEY_LEFTBRACKET },
	{ "BACKSLASH", KEY_BACKSLASH },
	{ "RIGHTBRACKET", KEY_RIGHTBRACKET },
	{ "APOSTROPHE", KEY_APOSTROPHE },
	{ "MISCELANIOUS1", KEY_MISCELANIOUS_1 },
	{ "MISCELANIOUS2", KEY_MISCELANIOUS_2 },
	{ "EQUAL", KEY_EQUAL },
	{ "COMMA", KEY_COMMA },
	{ "MINUS", KEY_MINUS },
	{ "PERIOD", KEY_PERIOD },
	{ "MOUSELEFT", MOUSE_LEFT },
	{ "MOUSERIGHT", MOUSE_RIGHT },
	{ "MOUSEMIDDLE", MOUSE_MIDDLE },
	{ "MOUSEX1", MOUSE_X1 },
	{ "MOUSEX2", MOUSE_X2 },
	{ "MOUSEBUTTON6", MOUSE_BUTTON6 },
	{ "MOUSEBUTTON7", MOUSE_BUTTON7 },
	{ "MOUSEBUTTON8", MOUSE_BUTTON8 },
	{ "MOUSEWHEELUP", MOUSE_WHEELUP },
	{ "MOUSEWHEELDOWN", MOUSE_WHEELDOWN },
	{ "FINAL", KEY_FINAL }
};

static key_t *keys;

static FILE *bindingsfile;
static const char *bindingsdir = "configs";
static const char *bindingsfilename = "bindings.cfg";
static char bindingsfullname[SYS_MAX_PATH];

static bool initialized;

/*
* Function: GetKeyName
* Gets the name of a key from its keycode
* 
* 	key: The keycode to get the name of
* 
* Returns: The text name of the key
*/
static const char *GetKeyName(const keycode_t key)
{
	for (int i=0; i<KEY_FINAL; i++)
	{
		if (keynames[i].key == key)
			return(keynames[i].name);
	}

	return(NULL);
}

/*
* Function: GetKeyFromName
* Gets the keycode from the name of a key
* 
* 	name: The name of the key
* 
* Returns: The keycode of the key name
*/
static keycode_t GetKeyFromName(const char *name)
{
	for (int i=0; i<KEY_FINAL; i++)
	{
		if (!strcmp(keynames[i].name, name))
			return(keynames[i].key);
	}

	return(KEY_UNKNOWN);
}

/*
* Function: SetBinding
* Sets a binding for a key, the binding is a command string that is executed by the command system
* 
* 	key: The keycode of the key to bind
* 	binding: The command string to bind to the key
*/
static void SetBinding(const keycode_t key, const char *binding)
{
	if (key == KEY_UNKNOWN)
		return;

	size_t len = Sys_Strlen(binding, CMD_MAX_STR_LEN);
	size_t currentlen = 0;

	if (keys[key].binding)
	{
		currentlen = Sys_Strlen(keys[key].binding, CMD_MAX_STR_LEN);

		if (strcmp(keys[key].binding, binding) == 0)
			return;

		// need to reallocate if the old binding is longer than the new one, otherwise just copy the new binding over the old one
		if (len > currentlen)
		{
			char *newbinding = MemCache_Alloc(len + 1);
			if (!newbinding)
			{
				Log_WriteSeq(LOG_ERROR, "failed to allocate memory for new key binding");
				return;
			}

			snprintf(newbinding, len + 1, "%s", binding);
			MemCache_Free(keys[key].binding);
			keys[key].binding = newbinding;
		}

		else
			snprintf(keys[key].binding, len + 1, "%s", binding);
	}

	else
	{
		keys[key].binding = MemCache_Alloc(len + 1);
		if (!keys[key].binding)
		{
			Log_WriteSeq(LOG_ERROR, "failed to allocate memory for new key binding");
			return;
		}

		snprintf(keys[key].binding, len + 1, "%s", binding);
	}
}

/*
* Function: WriteBindings
* Writes the key bindings to a file
* 
* 	bindings: The file to write the bindings to
*/
void WriteBindings(FILE *bindings)
{
	if (!bindings)
		return;

	for (int i=0; i<KEY_FINAL; i++)
	{
		if (keys[i].binding && keys[i].binding[0])
			fprintf(bindings, "bind %s %s\n", GetKeyName(i), keys[i].binding);
	}
}

/*
* Function: ReadBindings
* Reads the key bindings from a file
* 
* 	bindings: The file to read the bindings from
*/
static void ReadBindings(FILE *bindings)
{
	if (!bindings)
		return;

	char line[CMD_MAX_STR_LEN] = { 0 };
	while (fgets(line, sizeof(line), bindings))
	{
		char cmdline[CMD_MAX_STR_LEN] = { 0 };

		if (line[0] == '\n' || line[0] == '\r' || line[0] == '#')
			continue;

		char *saveptr = NULL;

		char *cmdname = Sys_Strtok(line, " ", &saveptr);
		char *args = Sys_Strtok(NULL, "\n\r", &saveptr);

		snprintf(cmdline, sizeof(cmdline), "%s %s", cmdname, args);

		Cmd_BufferCommand(CMD_EXEC_NOW, cmdline);
	}
}

/*
* Function: Bind_Cmd
* Binds a command to a key from the command line
* 
* 	args: The command arguments
*/
static void Bind_Cmd(const cmdargs_t *args)
{
	if (args->argc != 3)
	{
		Log_Write(LOG_INFO, "Usage: %s [keyname] [action]", args->argv[0]);
		return;
	}

	keycode_t key = GetKeyFromName(args->argv[1]);
	if (key == KEY_UNKNOWN)
	{
		Log_Write(LOG_ERROR, "Failed to bind key, invalid key name: %s", args->argv[1]);
		return;
	}

	SetBinding(key, args->argv[2]);
}

/*
* Function: Input_Init
* Initializes the input system
* 
* Returns: A boolean if initialization was successful or not
*/
bool Input_Init(void)
{
	if (initialized)
		return(true);

	keys = MemCache_Alloc(sizeof(key_t) * KEY_FINAL);
	if (!keys)
	{
		Log_WriteSeq(LOG_ERROR, "failed to allocate memory for keys");
		return(false);
	}

	memset(keys, 0, sizeof(key_t) * KEY_FINAL);

	for (int i=0; i<KEY_FINAL; i++)
	{
		keys[i].down = false;
		keys[i].repeats = 0;
		keys[i].binding = NULL;
	}

	Cmd_RegisterCommand("bind", Bind_Cmd, "binds a command to a key");

	if (!Sys_Mkdir(bindingsdir))
		return(false);

	snprintf(bindingsfullname, sizeof(bindingsfullname), "%s/%s", bindingsdir, bindingsfilename);

	if (!FileSys_FileExists(bindingsfullname))
	{
		bindingsfile = fopen(bindingsfullname, "w+");
		if (!bindingsfile)
		{
			Log_WriteSeq(LOG_ERROR, "bindings file does not exist and cannot be recreated: %s", bindingsfullname);
			return(false);
		}

		fclose(bindingsfile);	// close the opened file if created
		bindingsfile = NULL;
	}

	bindingsfile = fopen(bindingsfullname, "r");
	if (!bindingsfile)
	{
		Log_WriteSeq(LOG_ERROR, "failed to open bindings file: %s", bindingsfullname);
		return(false);
	}

	ReadBindings(bindingsfile);

	fclose(bindingsfile);
	bindingsfile = NULL;

	initialized = true;

	return(true);
}

/*
* Function: Input_Shutdown
* Shuts down the input system
*/
void Input_Shutdown(void)
{
	if (!initialized)
		return;

	Log_WriteSeq(LOG_INFO, "Shutting down input system");

	bindingsfile = fopen(bindingsfullname, "w");
	if (!bindingsfile)
	{
		Log_WriteSeq(LOG_ERROR, "failed to open bindings file for writing: %s", bindingsfullname);
		return;
	}

	WriteBindings(bindingsfile);

	fclose(bindingsfile);
	bindingsfile = NULL;

	for (int i=0; i<KEY_FINAL; i++)		// free all key bindings if they exist
	{
		if (keys[i].binding)
			MemCache_Free(keys[i].binding);
	}

	MemCache_Free(keys);
	keys = NULL;

	initialized = false;
}

/*
* Function: Input_ProcessKeyInput
* Processes key input and adds the bound command to the buffer if the key is pressed
* 
* 	key: The key that was pressed
* 	down: A boolean if the key was pressed or released
*/
void Input_ProcessKeyInput(const int key, bool down)
{
	if (key <= KEY_UNKNOWN || key >= KEY_FINAL)
		return;

	if (down)
	{
		if (!keys[key].down)
		{
			keys[key].down = true;
			keys[key].repeats = 0;
		}

		keys[key].repeats++;
	}

	else
		keys[key].down = false;

	if (down)
	{
		const char *binding = keys[key].binding;
		if (binding && binding[0])
			Cmd_BufferCommand(CMD_EXEC_APPEND, binding);
	}
}

/*
* Function: Input_ClearKeyStates
* Clears the key states for all keys, sets them to up and resets the repeat count
*/
void Input_ClearKeyStates(void)
{
	if (keys)
	{
		for (int i=0; i<KEY_FINAL; i++)
		{
			keys[i].down = false;
			keys[i].repeats = 0;
		}
	}
}
