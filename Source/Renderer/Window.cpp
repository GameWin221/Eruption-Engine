#include <Core/EnPch.hpp>
#include "Window.hpp"

en::Window* g_MainWindow;

namespace en
{
	Window::Window(const WindowInfo& windowInfo)
	{
		if (g_MainWindow)
			EN_ERROR("Window::Window() - Failed to open a window! There is an open window already.");

		g_MainWindow = this;

		m_WindowInfo = windowInfo;

		if(!glfwInit())
			EN_ERROR("Window::Window() - Failed to create the glfw context!");

		GLFWmonitor*       monitor = glfwGetPrimaryMonitor();
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

        if(!m_GLFWWindow)
            EN_ERROR("Window::Window() - Failed to create m_GLFWWindow!")
        
		EN_SUCCESS("Created a window")
	}
	Window::~Window()
	{
		g_MainWindow = nullptr;
		glfwDestroyWindow(m_GLFWWindow);

		EN_LOG("Closed the main window.");

		glfwTerminate();

		EN_LOG("Destroyed the glfw context.");
	}

	void Window::Close()
	{
		glfwSetWindowShouldClose(m_GLFWWindow, true);
		EN_LOG("Window should close within a frame...");
	}
	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	Window& Window::Get()
	{
		if (!g_MainWindow)
			EN_ERROR("Window::GetMainWindow() - g_MainWindow was a nullptr!");

		return *g_MainWindow;
	}
}