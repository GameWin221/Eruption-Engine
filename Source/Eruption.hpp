#pragma once

#ifndef EN_ERUPTIONCORE_HPP
#define EN_ERUPTIONCORE_HPP

#include <Rendering/Window.hpp>
#include <Rendering/Context.hpp>
#include <Rendering/Renderer.hpp>
#include <Utility/AssetManager.hpp>
#include <Utility/InputManager.hpp>

class Eruption
{
public:
	void Run();

private:
	void Init();
	void Update();
	void Render();

	void DrawImGuiUI();

	en::Window*		   m_Window;
	en::Context*	   m_Context;
	en::AssetManager*  m_AssetManager;
	en::InputManager*  m_Input;
	en::Renderer*	   m_Renderer;
	en::Camera*		   m_Camera;

	double m_TargetFPS = 144.0;

	double m_DeltaTime;
	float  m_fDeltaTime;
};

#endif