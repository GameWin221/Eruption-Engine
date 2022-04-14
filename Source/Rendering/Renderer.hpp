#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "VulkanRendererBackend.hpp"
#include <functional>

namespace en
{
	class Renderer
	{
	public:
		
		Renderer(RendererInfo& rendererInfo);
		~Renderer();
		
		void PrepareImGuiUI(std::function<void()> imGuiRenderCallback);

		void PrepareModel(Model* model);
		void RemoveModel (Model* model);
		void EnqueueModel(Model* model);

		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

		Renderer& GetRenderer();

		void ReloadRenderer();

	private:
		VulkanRendererBackend m_Backend;

		std::function<void()> m_ImGuiRenderCallback;
	};
}

#endif