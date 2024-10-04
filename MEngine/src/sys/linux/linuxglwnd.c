#include "../../common/common.h"
#include "linuxlocal.h"
#include "../../renderer/renderer.h"

bool GLWnd_Init(glwndparams_t params)
{
	RegisterCallbacks();
}

void GLWnd_Shutdown(void)
{
	Log_WriteSeq(LOG_INFO, "Shutting down OpenGL system");

	if (linuxstate.window)
	{
		glfwDestroyWindow(linuxstate.window);
		linuxstate.window = NULL;
	}

	glfwTerminate();
}

bool GLWnd_ChangeScreenParams(glwndparams_t params)
{
}

void GLWnd_SwapBuffers(void)
{
	glfwSwapBuffers(linuxstate.window);
}

void GLWnd_SetVSync(int vsync)
{
	glfwSwapInterval(vsync);
	linuxstate.swapinterval = vsync;
}

int GLWnd_GetVSync(void)
{
	return(linuxstate.swapinterval);
}
