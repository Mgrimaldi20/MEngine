#include "sys/sys.h"
#include "common/common.h"
#include "winlocal.h"

static int scancodes[] =
{
	KEY_UNKNOWN,		// unknown key 0x00
	KEY_UNKNOWN,		// left mouse button
	KEY_UNKNOWN,		// right mouse button
	KEY_CANCEL,			// control-break processing
	KEY_UNKNOWN,		// middle mouse button
	KEY_UNKNOWN,		// x1 mouse button .. x2 mouse button
	KEY_UNKNOWN,
	KEY_UNKNOWN,		// reserved
	KEY_BACKSPACE,		// backspace
	KEY_TAB,			// tab
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,
	KEY_CLEAR,			// clear
	KEY_ENTER,			// enter
	KEY_UNKNOWN,		// unasigned
	KEY_UNKNOWN,
	KEY_SHIFT,			// shift
	KEY_CONTROL,		// control
	KEY_ALT,			// alt
	KEY_PAUSE,			// pause
	KEY_CAPSLOCK,		// caps lock
	KEY_UNKNOWN,		// ime kana mode / ime hanguel mode
	KEY_UNKNOWN,		// ime on
	KEY_UNKNOWN,		// ime junja mode
	KEY_UNKNOWN,		// ime final mode
	KEY_UNKNOWN,		// ime hanja mode / ime kanji mode
	KEY_UNKNOWN,		// ime off
	KEY_ESCAPE,			// escape
	KEY_UNKNOWN,		// ime convert
	KEY_UNKNOWN,		// ime nonconvert
	KEY_UNKNOWN,		// ime accept
	KEY_UNKNOWN,		// ime mode change request
	KEY_SPACE,			// spacebar
	KEY_PAGEUP,			// page up
	KEY_PAGEDOWN,		// page down
	KEY_END,			// end
	KEY_HOME,			// home
	KEY_LEFT,			// left arrow
	KEY_UP,				// up arrow
	KEY_RIGHT,			// right arrow
	KEY_DOWN,			// down arrow
	KEY_SELECT,			// select
	KEY_PRINT,			// print
	KEY_EXEC,			// execute
	KEY_PRINTSCREEN,	// print screen
	KEY_INSERT,			// insert
	KEY_DELETE,			// delete
	KEY_HELP,			// help
	KEY_0,
	KEY_1,
	KEY_2,
	KEY_3,
	KEY_4,
	KEY_5,
	KEY_6,
	KEY_7,
	KEY_8,
	KEY_9,
	KEY_UNKNOWN,		// undefined
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,
	KEY_LEFTSPECIAL,	// left windows key
	KEY_RIGHTSPECIAL,	// right windows key
	KEY_APPLICATION,	// applications key
	KEY_UNKNOWN,		// reserved
	KEY_SLEEP,			// computer sleep key
	KEY_NUMPAD0,
	KEY_NUMPAD1,
	KEY_NUMPAD2,
	KEY_NUMPAD3,
	KEY_NUMPAD4,
	KEY_NUMPAD5,
	KEY_NUMPAD6,
	KEY_NUMPAD7,
	KEY_NUMPAD8,
	KEY_NUMPAD9,
	KEY_MULTIPLY,		// multiply key (numpad *)
	KEY_ADD,			// add key (numpad +)
	KEY_SEPARATOR,		// separator key
	KEY_SUBTRACT,		// subtract key (numpad -)
	KEY_DECIMAL,		// decimal key (numpad .)
	KEY_DIVIDE,			// divide key (numpad /)
	KEY_F1,
	KEY_F2,
	KEY_F3,
	KEY_F4,
	KEY_F5,
	KEY_F6,
	KEY_F7,
	KEY_F8,
	KEY_F9,
	KEY_F10,
	KEY_F11,
	KEY_F12,
	KEY_F13,
	KEY_F14,
	KEY_F15,
	KEY_F16,
	KEY_F17,
	KEY_F18,
	KEY_F19,
	KEY_F20,
	KEY_F21,
	KEY_F22,
	KEY_F23,
	KEY_F24,
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_NUMLOCK,		// num lock
	KEY_SCROLLLOCK,		// scroll lock
	KEY_UNKNOWN,		// oem specific
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,		// unassigned
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_LEFTSHIFT,		// left shift
	KEY_RIGHTSHIFT,		// right shift
	KEY_LEFTCONTROL,	// left control
	KEY_RIGHTCONTROL,	// right control
	KEY_LEFTALT,		// left alt
	KEY_RIGHTALT,		// right alt
	KEY_UNKNOWN,		// browser back
	KEY_UNKNOWN,		// browser forward
	KEY_UNKNOWN,		// browser refresh
	KEY_UNKNOWN,		// browser stop
	KEY_UNKNOWN,		// browser search
	KEY_UNKNOWN,		// browser favorites
	KEY_UNKNOWN,		// browser start and home
	KEY_UNKNOWN,		// volume mute
	KEY_UNKNOWN,		// volume down
	KEY_UNKNOWN,		// volume up
	KEY_UNKNOWN,		// next track
	KEY_UNKNOWN,		// previous track
	KEY_UNKNOWN,		// stop media
	KEY_UNKNOWN,		// play/pause media
	KEY_UNKNOWN,		// start mail
	KEY_UNKNOWN,		// select media
	KEY_UNKNOWN,		// start application 1 .. 2
	KEY_UNKNOWN,
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,
	KEY_SEMICOLON,		// misc keys: ; and :
	KEY_EQUAL,			// oem plus
	KEY_COMMA,			// oem comma
	KEY_MINUS,			// oem minus
	KEY_PERIOD,			// oem period
	KEY_SLASH,			// misc keys: / and ?
	KEY_GRAVE,			// misc keys: ` and ~
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_LEFTBRACKET,	// misc keys: [ and {
	KEY_BACKSLASH,		// misc keys: \ and |
	KEY_RIGHTBRACKET,	// misc keys: ] and }
	KEY_APOSTROPHE,		// misc keys: ' and "
	KEY_MISCELANIOUS_1,	// varies by keyboard
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,		// oem specific
	KEY_MISCELANIOUS_2,	// misc keys: < and >
	KEY_UNKNOWN,		// oem specific
	KEY_UNKNOWN,
	KEY_UNKNOWN,		// ime process
	KEY_UNKNOWN,		// oem specific
	KEY_UNKNOWN,		// unicode character packer
	KEY_UNKNOWN,		// unassigned
	KEY_UNKNOWN,		// oem specific
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,
	KEY_UNKNOWN,		// attn
	KEY_UNKNOWN,		// crsel
	KEY_UNKNOWN,		// exsel
	KEY_UNKNOWN,		// ereof
	KEY_UNKNOWN,		// play
	KEY_UNKNOWN,		// zoom
	KEY_UNKNOWN,		// reserved
	KEY_UNKNOWN,		// pa1
	KEY_UNKNOWN,		// clear
	KEY_FINAL
};

/*
* Function: MapWin32Key
* Maps a Win32 key to a common key code
* 
* 	key: The Win32 key code, stored in the WPARAM of the Windows message
* 
* Returns: The common key code
*/
keycode_t MapWin32Key(WPARAM key)
{
	if (key < 0 || key >= (sizeof(scancodes) / sizeof(scancodes[0])))
		return(KEY_UNKNOWN);

	return(scancodes[key]);
}
