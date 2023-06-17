#pragma once

#ifndef EN_WINDOW_HPP
#define EN_WINDOW_HPP

#include <Core/Log.hpp>

#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>

#include <string>
#include <iostream>
#include <glm.hpp>

namespace en
{
	class Window
	{
	public:
		Window(
			const std::string& title = "Unnamed Default Window",
			const glm::ivec2& size = glm::ivec2(1024, 720),
			const bool fullscreen = false,
			const bool resizable = false
		);

		~Window();

		static Window& Get();

		void Close();
		void PollEvents();

		void SetTitle(const std::string& title);
		void SetSize(const glm::ivec2& size);

		void SetFullscreen(const bool fullscreen);
	
		void SetResizeCallback(GLFWframebuffersizefun callback);

		glm::ivec2 GetFramebufferSize();

		const bool IsOpen() const;

		const std::string& GetTitle() const;
		const glm::ivec2& GetSize();

		const bool GetIsFullscreen() const;
		const bool GetIsResizable() const;

		GLFWwindow* GetNativeHandle() const { return m_NativeHandle; };

	private:
		GLFWwindow* m_NativeHandle;

		std::string m_Title;
		glm::ivec2 m_Size;

		bool m_IsFullscreen;
		bool m_IsResizable;
	};
}
#endif