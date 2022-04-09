#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "VulkanRendererBackend.hpp"

namespace en
{
	class Renderer
	{
	public:
		
		Renderer(RendererInfo& rendererInfo);
		~Renderer();
		
		void PrepareModel(Model* model);
		void RemoveModel (Model* model);
		void EnqueueModel(Model* model);

		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

		Renderer& GetRenderer();

	private:
		VulkanRendererBackend m_Backend;
	};
}

#endif