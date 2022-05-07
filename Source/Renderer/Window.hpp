#pragma once

#ifndef EN_WINDOW_HPP
#define EN_WINDOW_HPP

namespace en
{
	struct WindowInfo
	{
		std::string title = "Unnamed Default Window";

		glm::ivec2 size = glm::ivec2(1024, 720);

		bool fullscreen = false;

		// Works only if `fullscreen = false`
		bool resizable = false;
	};

	class Window
	{
	public:
		Window(WindowInfo& windowInfo);
		~Window();

		void Close();
		void PollEvents();

		GLFWwindow*  m_GLFWWindow;

		static Window&     GetMainWindow();
		const  WindowInfo& GetInfo() const { return m_WindowInfo; };

		const bool IsOpen() const { return !glfwWindowShouldClose(m_GLFWWindow); };

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

	private:
		WindowInfo m_WindowInfo;
	};
}
#endif