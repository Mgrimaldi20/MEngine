#include <stdio.h>
#include <math.h>
#include "common/common.h"
#include "renderer/renderer.h"
#include "posixlocal.h"

/*
* Function: KeyCallback
* Callback for key events, queues the key event to the event system
* 
* 	window: The window that received the event
* 	key: The key that was pressed
* 	scancode: The system-specific scancode of the key
* 	action: The action that was taken
* 	mods: Any modifier keys that were pressed
*/
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

/*
* Function: MouseMoveCallback
* Callback for mouse move events, queues the mouse event to the event system with the mouse position, relative to top-left corner
* 
* 	window: The window that received the event
* 	xpos: The new x-coordinate of the cursor
* 	ypos: The new y-coordinate of the cursor
*/
static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
	int posx = (int)xpos;
	int posy = (int)ypos;

	if (posx >= 0 && posx < glstate.width && posy >= 0 && posy < glstate.height)
		Event_QueueEvent(EVENT_MOUSE, posx, posy);
}

/*
* Function: MouseButtonCallback
* Callback for mouse button events, queues the mouse event to the event system
* 
* 	window: The window that received the event
* 	button: The button that was pressed
* 	action: The action that was taken
* 	mods: Any modifier keys that were pressed
*/
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

/*
* Function: MouseScrollCallback
* Callback for mouse scroll events, queues the mouse event to the event system
* 
* 	window: The window that received the event
* 	xoffset: The scroll offset along the x-axis, unused at the moment
* 	yoffset: The scroll offset along the y-axis, this is the value that is used for the mouse wheel
*/
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

/*
* Function: WindowSizeCallback
* Callback for window size events, resizes the window to the minimum size if the window is too small
* 
* 	window: The window that received the event
* 	width: The new width of the window
* 	height: The new height of the window
*/
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

/*
* Function: FramebufferSizeCallback
* Callback for framebuffer size events, resizes the viewport to the new size, all controlled by GLFW
* 
* 	window: The window that received the event
* 	width: The new width of the framebuffer
* 	height: The new height of the framebuffer
*/
static void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
	glstate.width = width;
	glstate.height = height;

	Cmd_BufferCommand(CMD_EXEC_NOW, "sizeviewport");
}

/*
* Function: WindowCloseCallback
* Callback for window close events, sets the window to close
* 
* 	window: The window that received the event
*/
static void WindowCloseCallback(GLFWwindow *window)
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}

/*
* Function: WindowFocusCallback
* Callback for window focus events, clears the key states when losing focus or regaining focus
* 
* 	window: The window that received the event
* 	focused: A boolean if the window is focused or not
*/
static void WindowFocusCallback(GLFWwindow *window, int focused)
{
	Input_ClearKeyStates();
}

/*
* Function: WindowIconifyCallback
* Callback for window iconification events, clears the key states when the window is iconified
* 
* 	window: The window that received the event
* 	iconified: A boolean if the window is iconified or not
*/
static void WindowIconifyCallback(GLFWwindow *window, int iconified)
{
	Input_ClearKeyStates();
}

/*
* Function: RegisterCallbacks
* Registers all the callbacks for the window
* 
* 	window: The window to register the callbacks to
*/
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
