#include "Window.hpp"

en::Window* g_MainWindow = nullptr;

namespace en
{
	Window::Window(const std::string& title, const glm::ivec2& size, const bool fullscreen, const bool resizable)
		: m_Title(title), m_Size(size), m_IsFullscreen(fullscreen), m_IsResizable(resizable)
	{
		if (g_MainWindow)
			EN_ERROR("Window::Window() - Failed to open a window! There is an open window already.");

		if (!glfwInit())
			EN_ERROR("Window::Window() - Failed to create the glfw context!");

		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RED_BITS, mode->redBits);
		glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
		glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
		glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

		// Fullscreen
		if (m_IsFullscreen)
			m_NativeHandle = glfwCreateWindow(mode->width, mode->height, m_Title.c_str(), monitor, nullptr);

		// Windowed
		else
		{
			glfwWindowHint(GLFW_RESIZABLE, m_IsResizable);
			m_NativeHandle = glfwCreateWindow(m_Size.x, m_Size.y, m_Title.c_str(), nullptr, nullptr);
		}

		if (!m_NativeHandle)
			EN_ERROR("Window::Window() - Failed to create m_GLFWWindow!")

		g_MainWindow = this;

		EN_SUCCESS("Created a window")
	}
	Window::~Window()
	{
		glfwDestroyWindow(m_NativeHandle);

		g_MainWindow = nullptr;

		EN_LOG("Closed the main window.");

		glfwTerminate();

		EN_LOG("Destroyed the glfw context.");
	}

	Window& Window::Get()
	{
		if (!g_MainWindow)
			EN_ERROR("Window::Get() - g_MainWindow was a nullptr!");

		return *g_MainWindow;
	}

	void Window::Close()
	{
		glfwSetWindowShouldClose(m_NativeHandle, true);
	}
	void Window::PollEvents()
	{
		glfwPollEvents();
	}

	void Window::SetTitle(const std::string& title)
	{
		glfwSetWindowTitle(m_NativeHandle, title.c_str());
		m_Title = title;
	}

	void Window::SetSize(const glm::ivec2& size)
	{
		glfwSetWindowSize(m_NativeHandle, size.x, size.y);
		m_Size = size;
	}

	void Window::SetFullscreen(const bool fullscreen)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* mode = glfwGetVideoMode(monitor);

		if (fullscreen) 
			glfwSetWindowMonitor(m_NativeHandle, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
		else
			glfwSetWindowMonitor(m_NativeHandle, nullptr, 0, 0, 1080, 720, mode->refreshRate);

		m_IsFullscreen = fullscreen;
	}

	const bool Window::IsOpen() const
	{
		return !glfwWindowShouldClose(m_NativeHandle);
	}

	const std::string& Window::GetTitle() const
	{
		return m_Title;
	}

	const glm::ivec2& Window::GetSize()
	{
		int w = 0, h = 0;
		glfwGetWindowSize(m_NativeHandle, &w, &h);

		return glm::ivec2(w, h);
	}

	glm::ivec2 Window::GetFramebufferSize()
	{
		int w = 0, h = 0;
		glfwGetFramebufferSize(m_NativeHandle, &w, &h);

		return glm::ivec2(w, h);
	}

	void Window::SetResizeCallback(GLFWframebuffersizefun callback)
	{
		glfwSetWindowSizeCallback(m_NativeHandle, callback);
	}

	const bool Window::GetIsFullscreen() const
	{
		return m_IsFullscreen;
	}

	const bool Window::GetIsResizable() const
	{
		return m_IsResizable;
	}
}