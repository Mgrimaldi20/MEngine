#include "sys/sys.h"
#include "common/common.h"
#include "common/input.h"
#include "posixlocal.h"

static int scancodes[] =
{
	KEY_UNKNOWN
};

int MapGLFWKey(int key)
{
	if (key < 0 || key >= (sizeof(scancodes) / sizeof(scancodes[0])) || key == GLFW_KEY_UNKNOWN)
		return(KEY_UNKNOWN);

	return(scancodes[key]);
}
