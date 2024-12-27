#include "sys/sys.h"
#include "common/common.h"
#include "winlocal.h"

static int scancodes[] =
{
	0,		// unknown key 0x00
	'0',
	'1',
	'2',
	'3',
	'4',
	'5',
	'6',
	'7',
	'8',
	'9',
	'A',
	'B',
	'C',
	'D',
	'E',
	'F',
	'G',
	'H',
	'I',
	'J',
	'K',
	'L',
	'M',
	'N',
	'O',
	'P',
	'Q',
	'R',
	'S',
	'T',
	'U',
	'V',
	'W',
	'X',
	'Y',
	'Z'
};

keycode_t MapKey(WPARAM key)
{
	for (int i=0; i<KEY_FINAL; ++i)
	{
		if (scancodes[i] == key)
			return(i);
	}

	return(KEY_UNKNOWN);
}
