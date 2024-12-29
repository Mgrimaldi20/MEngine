#include <stdio.h>
#include "renderer/renderer.h"
#include "posixlocal.h"

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	keycode_t keycode = KEY_UNKNOWN;

	switch (action)
	{
		case GLFW_PRESS:
			keycode = MapGLFWKey(key);

			if (key == KEY_PRINTSCREEN)
				break;

#if defined(MENGINE_DEBUG)
			Common_Printf("%d: Key down: Translated: %d, GLFW keycode: %d\n", action, keycode, key);
#endif
			break;

		case GLFW_RELEASE:
			keycode = MapGLFWKey(key);

			if (key == KEY_PRINTSCREEN)
				break;

#if defined(MENGINE_DEBUG)
			Common_Printf("%d: Key up: Translated: %d, GLFW keycode: %d\n", action, keycode, key);
#endif
			break;

		case GLFW_REPEAT:
			break;
	}
}

static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
	mousecode_t code = MOUSE_UNKNOWN;

	switch (action)
	{
		case GLFW_PRESS:
			code = MapGLFWMouse(button);

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button down: %d\n", code);
#endif
			break;

		case GLFW_RELEASE:
			code = MapGLFWMouse(button);

#if defined(MENGINE_DEBUG)
			Common_Printf("Mouse button up: %d\n", code);
#endif
			break;

		case GLFW_REPEAT:		// do mouse buttons have repeats?
			break;
	}
}

static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
}

static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
}

static void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
	if (!glstate.initialized || width <= 0 || height <= 0)
		return;

	int minwidth = 320;
	int minheight = 240;

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

	glstate.viewportsized = true;
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
