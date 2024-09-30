#include "linuxlocal.h"

static void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
}

static void MouseMoveCallback(GLFWwindow *window, double xpos, double ypos)
{
}

static void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
}

static void MouseScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
{
}

static void CharCallback(GLFWwindow *window, unsigned int codepoint)
{
}

static void WindowSizeCallback(GLFWwindow *window, int width, int height)
{
}

static void FramebufferSizeCallback(GLFWwindow *window, int width, int height)
{
}

static void WindowCloseCallback(GLFWwindow *window)
{
}

static void WindowFocusCallback(GLFWwindow *window, int focused)
{
}

static void WindowIconifyCallback(GLFWwindow *window, int iconified)
{
}

void RegisterCallbacks(void)
{
	glfwSetKeyCallback(linuxstate.window, KeyCallback);
	glfwSetCursorPosCallback(linuxstate.window, MouseMoveCallback);
	glfwSetMouseButtonCallback(linuxstate.window, MouseButtonCallback);
	glfwSetScrollCallback(linuxstate.window, MouseScrollCallback);
	glfwSetCharCallback(linuxstate.window, CharCallback);
	glfwSetWindowSizeCallback(linuxstate.window, WindowSizeCallback);
	glfwSetFramebufferSizeCallback(linuxstate.window, FramebufferSizeCallback);
	glfwSetWindowCloseCallback(linuxstate.window, WindowCloseCallback);
	glfwSetWindowFocusCallback(linuxstate.window, WindowFocusCallback);
	glfwSetWindowIconifyCallback(linuxstate.window, WindowIconifyCallback);
}
