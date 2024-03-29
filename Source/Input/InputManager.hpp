#pragma once

#ifndef EN_INPUT_MANAGER
#define EN_INPUT_MANAGER

#include <Renderer/Window.hpp>

#include <unordered_map>

namespace en
{
	enum struct Key
	{
		Q = GLFW_KEY_Q,
		W = GLFW_KEY_W,
		E = GLFW_KEY_E,
		R = GLFW_KEY_R,
		T = GLFW_KEY_T,
		Y = GLFW_KEY_Y,
		U = GLFW_KEY_U,
		I = GLFW_KEY_I,
		O = GLFW_KEY_O,
		P = GLFW_KEY_P,
		A = GLFW_KEY_A,
		S = GLFW_KEY_S,
		D = GLFW_KEY_D,
		F = GLFW_KEY_F,
		G = GLFW_KEY_G,
		H = GLFW_KEY_H,
		J = GLFW_KEY_J,
		K = GLFW_KEY_K,
		L = GLFW_KEY_L,
		Z = GLFW_KEY_Z,
		X = GLFW_KEY_X,
		C = GLFW_KEY_C,
		V = GLFW_KEY_V,
		B = GLFW_KEY_B,
		N = GLFW_KEY_N,
		M = GLFW_KEY_M,

		Space  = GLFW_KEY_SPACE,
		LCtrl  = GLFW_KEY_LEFT_CONTROL,
		RCtrl  = GLFW_KEY_RIGHT_CONTROL,
		LShift = GLFW_KEY_LEFT_SHIFT,
		RShift = GLFW_KEY_RIGHT_SHIFT,
		Tab    = GLFW_KEY_TAB,
		Escape = GLFW_KEY_ESCAPE,

		Zero  = GLFW_KEY_0,
		One   = GLFW_KEY_1,
		Two   = GLFW_KEY_2,
		Three = GLFW_KEY_3,
		Four  = GLFW_KEY_4,
		Five  = GLFW_KEY_5,
		Six   = GLFW_KEY_6,
		Seven = GLFW_KEY_7,
		Eight = GLFW_KEY_8,
		Nine  = GLFW_KEY_9,

		Up    = GLFW_KEY_UP,
		Down  = GLFW_KEY_DOWN,
		Right = GLFW_KEY_RIGHT,
		Left  = GLFW_KEY_LEFT
	};

	enum struct Button
	{
		Left   = GLFW_MOUSE_BUTTON_LEFT,
		Right  = GLFW_MOUSE_BUTTON_RIGHT,
		Middle = GLFW_MOUSE_BUTTON_MIDDLE
	};

	enum struct CursorMode
	{
		Locked = GLFW_CURSOR_DISABLED,
		Free   = GLFW_CURSOR_NORMAL
	};

	enum struct InputState
	{
		Pressed,
		Down,
		Released
	};

	class InputManager
	{
	public:
		InputManager();
		
		void UpdateMouse();
		void UpdateInput();

		bool IsKey		  (Key key	    , InputState inputState = InputState::Down);
		bool IsMouseButton(Button button, InputState inputState = InputState::Down);

		void SetCursorMode(CursorMode cursorMode);

		float m_MouseSensitivity = 1.0f;

		const glm::vec2  GetMousePosition()    const;
		const glm::vec2  GetMouseVelocity()    const { return m_MouseVel * m_MouseSensitivity; };
		const float		 GetScrollWheelDelta() const { return m_ScrollVel; };
		const CursorMode GetCursorMode()       const { return m_CursorMode; };

	private:
		CursorMode m_CursorMode;

		glm::vec2 m_LastMousePos;
		glm::vec2 m_MouseVel;
		float m_ScrollVel;

		// True / False - Down / Up
		std::unordered_map<int, bool> m_LastKeyStates;
		std::unordered_map<int, bool> m_LastButtonStates;

		GLFWwindow* m_Window;
	};
}
#endif