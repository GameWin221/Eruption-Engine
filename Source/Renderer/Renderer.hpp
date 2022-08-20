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

		void Update();

		void WaitForGPUIdle();

		void Render();

		void SetMainCamera(Camera* camera);
		Camera* GetMainCamera();

		void SetShadowCascadesWeight(float weight);
		const float& GetShadowCascadesWeight() const;

		static Renderer* Get();
		void ReloadRenderer();

		RendererBackend::PostProcessingParams& GetPPParams();

		void SetUIRenderCallback(std::function<void()> callback);
		void SetDebugMode(int& mode);
		void SetVSync(bool vSync);

	private:
		std::unique_ptr<RendererBackend> m_Backend;
	};
}

#endif