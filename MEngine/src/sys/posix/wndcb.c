#include <stdio.h>
#include <math.h>
#include "common/common.h"
#include "renderer/renderer.h"
#include "posixlocal.h"

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	keycode_t keycode = KEY_UNKNOWN;

	switch (action)
	{
		case GLFW_PRESS:
		{
			keycode = MapGLFWKey(key);

			if (key == KEY_PRINTSCREEN)
				break;

			Event_QueueEvent(EVENT_KEY, keycode, EVENT_TYPE_KEYDOWN);
			break;
		}

		case GLFW_RELEASE:
		{
			keycode = MapGLFWKey(key);

			if (key == KEY_PRINTSCREEN)
				break;

			Event_QueueEvent(EVENT_KEY, keycode, EVENT_TYPE_KEYUP);
			break;
		}

		case GLFW_REPEAT:
			keycode = MapGLFWKey(key);
			Event_QueueEvent(EVENT_KEY, keycode, EVENT_TYPE_KEYDOWN);
			break;
	}
}

static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
	int posx = (int)xpos;
	int posy = (int)ypos;

	if (posx >= 0 && posx < glstate.width && posy >= 0 && posy < glstate.height)
		Event_QueueEvent(EVENT_MOUSE, posx, posy);
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	keycode_t keycode = KEY_UNKNOWN;

	switch (action)
	{
		case GLFW_PRESS:
		{
			switch (button)
			{
				case GLFW_MOUSE_BUTTON_LEFT:
					keycode = MOUSE_LEFT;
					break;

				case GLFW_MOUSE_BUTTON_RIGHT:
					keycode = MOUSE_RIGHT;
					break;

				case GLFW_MOUSE_BUTTON_MIDDLE:
					keycode = MOUSE_MIDDLE;
					break;

				case GLFW_MOUSE_BUTTON_4:
					keycode = MOUSE_BUTTON4;
					break;

				case GLFW_MOUSE_BUTTON_5:
					keycode = MOUSE_BUTTON5;
					break;

				case GLFW_MOUSE_BUTTON_6:
					keycode = MOUSE_BUTTON6;
					break;

				case GLFW_MOUSE_BUTTON_7:
					keycode = MOUSE_BUTTON7;
					break;

				case GLFW_MOUSE_BUTTON_8:
					keycode = MOUSE_BUTTON8;
					break;

				default:
					return;
			}

			Event_QueueEvent(EVENT_KEY, keycode, EVENT_TYPE_KEYDOWN);
			break;
		}

		case GLFW_RELEASE:
		{
			switch (button)
			{
				case GLFW_MOUSE_BUTTON_LEFT:
					keycode = MOUSE_LEFT;
					break;

				case GLFW_MOUSE_BUTTON_RIGHT:
					keycode = MOUSE_RIGHT;
					break;

				case GLFW_MOUSE_BUTTON_MIDDLE:
					keycode = MOUSE_MIDDLE;
					break;

				case GLFW_MOUSE_BUTTON_4:
					keycode = MOUSE_BUTTON4;
					break;

				case GLFW_MOUSE_BUTTON_5:
					keycode = MOUSE_BUTTON5;
					break;

				case GLFW_MOUSE_BUTTON_6:
					keycode = MOUSE_BUTTON6;
					break;

				case GLFW_MOUSE_BUTTON_7:
					keycode = MOUSE_BUTTON7;
					break;

				case GLFW_MOUSE_BUTTON_8:
					keycode = MOUSE_BUTTON8;
					break;

				default:
					return;
			}

			Event_QueueEvent(EVENT_KEY, keycode, EVENT_TYPE_KEYUP);
			break;
		}
	}
}

static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
	keycode_t key = yoffset < 0 ? MOUSE_WHEELDOWN : MOUSE_WHEELUP;
	int delta = (int)fabs(yoffset);

	while (delta-- > 0)
	{
		Event_QueueEvent(EVENT_KEY, key, EVENT_TYPE_KEYDOWN);
		Event_QueueEvent(EVENT_KEY, key, EVENT_TYPE_KEYUP);
	}
}

static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
}

static void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
	if (!glstate.initialized || width <= 0 || height <= 0)
		return;

	int minwidth = Render_GetMinWidth();
	int minheight = Render_GetMinHeight();

	if (width < minwidth)
		width = minwidth;

	if (height < minheight)
		height = minheight;

	glfwSetWindowSize(window, width, height);
}

static void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	glstate.width = width;		// let GLFW control the framebuffer size
	glstate.height = height;

	Cmd_BufferCommand(CMD_EXEC_NOW, "sizeviewport");
}

static void WindowCloseCallback(GLFWwindow *window)
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

static void WindowFocusCallback(GLFWwindow *window, int focused)
{
}

static void WindowIconifyCallback(GLFWwindow *window, int iconified)
{
}

void RegisterCallbacks(GLFWwindow *window)
{
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetCursorPosCallback(window, MouseMoveCallback);
	glfwSetMouseButtonCallback(window, MouseButtonCallback);
	glfwSetScrollCallback(window, MouseScrollCallback);
	glfwSetCharCallback(window, CharCallback);
	glfwSetWindowSizeCallback(window, WindowSizeCallback);
	glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
	glfwSetWindowCloseCallback(window, WindowCloseCallback);
	glfwSetWindowFocusCallback(window, WindowFocusCallback);
	glfwSetWindowIconifyCallback(window, WindowIconifyCallback);
}
