#include <string.h>
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
	const char *binding;
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

static bool initialized;

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

	initialized = true;

	return(true);
}

void Input_Shutdown(void)
{
	if (!initialized)
		return;

	for (int i=0; i<KEY_FINAL; i++)		// free all key bindings if they exist
	{
		if (keys[i].binding)
			MemCache_Free(keys[i].binding);
	}

	MemCache_Free(keys);
	keys = NULL;

	initialized = false;
}
