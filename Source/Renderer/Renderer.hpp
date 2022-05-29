#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include <Renderer/RendererBackend.hpp>

namespace en
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		
		Scene* GetScene();
		void BindScene(Scene* scene);
		void UnbindScene();

		void WaitForGPUIdle();
		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

		Renderer& GetRenderer();
		void ReloadRenderer();

		VulkanRendererBackend::PostProcessingParams& GetPPParams();

		void SetUIRenderCallback(std::function<void()> callback);
		void SetDebugMode(int& mode);
		void SetVSync(bool vSync);
		
	private:
		std::unique_ptr<VulkanRendererBackend> m_Backend;
	};
}

#endif