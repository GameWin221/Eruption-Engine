#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include <Renderer/RendererBackend.hpp>

namespace en
{
	class Renderer
	{
	public:
		Renderer(RendererInfo& rendererInfo);
		~Renderer();

		void BindScene(Scene* scene);
		void UnbindScene();

		Scene* GetScene();
		
		void WaitForGPUIdle();
		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

		Renderer& GetRenderer();

		void ReloadRenderer();

		void SetUIRenderCallback(std::function<void()> callback);
		void SetDebugMode(int& mode);

		std::array<PointLight, MAX_LIGHTS>& GetPointLights();

	private:
		VulkanRendererBackend m_Backend;
	};
}

#endif