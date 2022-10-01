#pragma once

#ifndef EN_ERUPTIONCORE_HPP
#define EN_ERUPTIONCORE_HPP

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Renderer.hpp>
#include <Assets/AssetManager.hpp>
#include <Input/InputManager.hpp>
#include <Editor/EditorLayer.hpp>

class Eruption
{
public:
	void Run();

private:
	void Init();
	void Update();
	void Render();

	void UpdateExampleScene();
	void CreateExampleScene();

	en::EditorLayer*   m_Editor = nullptr;
	en::Window*		   m_Window  = nullptr;
	en::Context*	   m_Context = nullptr;
	en::AssetManager*  m_AssetManager = nullptr;
	en::InputManager*  m_Input    = nullptr;
	en::Renderer*	   m_Renderer = nullptr;
	en::Camera*		   m_Camera   = nullptr;

	en::Scene* m_ExampleScene;

	double m_DeltaTime;
};

#endif