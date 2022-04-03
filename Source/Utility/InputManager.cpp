#include "EnPch.hpp"
#include "Utility/InputManager.hpp"

namespace en
{
	InputManager::InputManager()
	{
		m_Window = Window::GetMainWindow().m_GLFWWindow;

		m_LastMousePos = GetMousePosition();

		m_MouseSensitivity = 1.0f;

		m_MouseVel = glm::vec2(0);

		m_CursorMode = CursorMode::Free;
	}
	
	void InputManager::UpdateInput()
	{
		m_MouseVel = GetMousePosition() - m_LastMousePos;

		m_LastMousePos = GetMousePosition();
	}

	bool InputManager::IsKeyDown(const Key& key)
	{
		return glfwGetKey(m_Window, static_cast<int>(key));
	}
	bool InputManager::IsMouseButtonDown(const Button& button)
	{
		return glfwGetMouseButton(m_Window, static_cast<int>(button));
	}

	void InputManager::SetCursorMode(const CursorMode& cursorMode)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, static_cast<int>(cursorMode));
		m_CursorMode = cursorMode;
	}

	const glm::vec2 InputManager::GetMousePosition() const
	{
		double mX = 0.0, mY = 0.0;

		glfwGetCursorPos(m_Window, &mX, &mY);

		return glm::vec2(mX, mY);
	}
}