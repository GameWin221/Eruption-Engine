#pragma once

#ifndef EN_ERUPTIONCORE_HPP
#define EN_ERUPTIONCORE_HPP

#include <Rendering/Window.hpp>
#include <Rendering/Context.hpp>
#include <Rendering/Renderer.hpp>
#include <Utility/AssetImporter.hpp>

class Eruption
{
public:
	void Run();

private:
	void Init();
	void Update();
	void Render();

	en::Window*		   m_Window;
	en::Context*	   m_Context;
	en::AssetImporter* m_AssetImporter;
	en::Renderer*	   m_Renderer;
	en::Camera*		   m_Camera;

	std::chrono::high_resolution_clock::time_point m_LastFrame = std::chrono::high_resolution_clock::now();

	std::vector<en::Mesh*> m_Meshes;
};

#endif