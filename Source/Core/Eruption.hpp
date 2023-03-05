#pragma once

#ifndef EN_ERUPTIONCORE_HPP
#define EN_ERUPTIONCORE_HPP

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Core/Types.hpp>
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

	void CreateExampleScene();

	en::Scope<en::EditorLayer>  m_Editor;
	en::Scope<en::Window>		m_Window;
	en::Scope<en::Context>		m_Context;
	en::Scope<en::AssetManager> m_AssetManager;
	en::Scope<en::InputManager> m_Input;
	en::Scope<en::Renderer>	    m_Renderer;

	en::Handle<en::Camera> m_Camera;
	en::Handle<en::Scene>  m_ExampleScene;
};

#endif