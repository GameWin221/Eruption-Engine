#include <Core/EnPch.hpp>
#include "InputManager.hpp"

namespace en
{
	InputManager::InputManager()
	{
		m_Window = Window::Get().m_GLFWWindow;

		m_LastMousePos = GetMousePosition();

		m_MouseSensitivity = 1.0f;

		m_MouseVel = glm::vec2(0);

		m_CursorMode = CursorMode::Free;

		glfwSetInputMode(m_Window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

		// GLFW_KEYs range from 32 to 348
		for (int key = 32; key <= 348; key++)
			m_LastKeyStates[key] = false;

		// GLFW_MOUSE_BUTTONs range from 0 to 7
		for (int button = 0; button <= 7; button++)
			m_LastButtonStates[button] = false;

		EN_SUCCESS("Created the input manager");
	}
	
	void InputManager::UpdateMouse()
	{
		m_MouseVel = GetMousePosition() - m_LastMousePos;

		m_LastMousePos = GetMousePosition();
	}

	void InputManager::UpdateInput()
	{
		// GLFW_KEYs range from 32 to 348
		for (int key = 32; key <= 348; key++)
			m_LastKeyStates.at(key) = glfwGetKey(m_Window, key);

		// GLFW_MOUSE_BUTTONs range from 0 to 7
		for (int button = 0; button <= 7; button++)
			m_LastButtonStates[button] = glfwGetMouseButton(m_Window, button);
	}

	bool InputManager::IsKey(const Key& key, const InputState& inputState)
	{
		const int keyState = glfwGetKey(m_Window, (int)key);

		switch (inputState)
		{
		case InputState::Pressed:
			return keyState == GLFW_PRESS && !m_LastKeyStates.at((int)key);
			break;

		case InputState::Released:
			return keyState == GLFW_RELEASE && m_LastKeyStates.at((int)key);
			break;

		case InputState::Down:
			return keyState == GLFW_PRESS;
			break;
		}
	}
	bool InputManager::IsMouseButton(const Button& button, const InputState& inputState)
	{
		const int buttonState = glfwGetMouseButton(m_Window, (int)button);

		switch (inputState)
		{
		case InputState::Pressed:
			return buttonState == GLFW_PRESS && !m_LastButtonStates.at((int)button);
			break;

		case InputState::Released:
			return buttonState == GLFW_RELEASE && m_LastButtonStates.at((int)button);
			break;

		case InputState::Down:
			return buttonState == GLFW_PRESS;
			break;
		}
	}

	void InputManager::SetCursorMode(const CursorMode& cursorMode)
	{
		glfwSetInputMode(m_Window, GLFW_CURSOR, (int)cursorMode);
		m_CursorMode = cursorMode;
	}

	const glm::vec2 InputManager::GetMousePosition() const
	{
		double mX = 0.0, mY = 0.0;

		glfwGetCursorPos(m_Window, &mX, &mY);

		return glm::vec2(mX, mY);
	}
}