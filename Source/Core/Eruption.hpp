#pragma once

#ifndef EN_ERUPTIONCORE_HPP
#define EN_ERUPTIONCORE_HPP

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Renderer.hpp>
#include <Assets/AssetManager.hpp>
#include <Input/InputManager.hpp>
#include <Core/UILayer.hpp>

class Eruption
{
public:
	void Run();

private:
	void Init();
	void Update();
	void Render();

	en::UILayer*	   m_UILayer;
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