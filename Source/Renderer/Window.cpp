#include <Core/EnPch.hpp>
#include "Window.hpp"

en::Window* g_MainWindow;

namespace en
{
	Window::Window(WindowInfo& windowInfo)
	{
		if (g_MainWindow)
			EN_ERROR("Failed to open a window! There is an open window already.");

		g_MainWindow = this;

		m_WindowInfo = windowInfo;

		glfwInit();

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_CLIENT_API  , GLFW_NO_API);
		glfwWindowHint(GLFW_RED_BITS	, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS  , mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS   , mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		// Fullscreen
		if (m_WindowInfo.fullscreen)
			m_GLFWWindow = glfwCreateWindow(mode->width, mode->height, m_WindowInfo.title.c_str(), monitor, nullptr);

		// Windowed
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, m_WindowInfo.resizable);
			m_GLFWWindow = glfwCreateWindow(m_WindowInfo.size.x, m_WindowInfo.size.y, m_WindowInfo.title.c_str(), nullptr, nullptr);
		}
	}
	Window::~Window()
	{
		glfwDestroyWindow(m_GLFWWindow);

		g_MainWindow = nullptr;

		glfwTerminate();
	}

	void Window::Close()
	{
		glfwSetWindowShouldClose(m_GLFWWindow, true);
	}
	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	Window& Window::GetMainWindow()
	{
		return *g_MainWindow;
	}
}