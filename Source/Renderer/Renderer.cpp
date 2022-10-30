#include <Core/EnPch.hpp>
#include "Renderer.hpp"

namespace en
{
	Renderer* g_CurrentRenderer;

	Renderer::Renderer()
	{
		g_CurrentRenderer = this;

		m_Backend = std::make_unique<RendererBackend>();

		m_Backend->Init();

		EN_SUCCESS("Created the renderer");
	}
	Renderer::~Renderer(){}

	void Renderer::BindScene(Scene* scene)
	{
		m_Backend->BindScene(scene);
	}
	void Renderer::UnbindScene()
	{
		m_Backend->UnbindScene();
	}

	void Renderer::Update()
	{
		m_Backend->UpdateLights();
	}

	Scene* Renderer::GetScene()
	{
		return m_Backend->GetScene();
	}

	void Renderer::WaitForGPUIdle()
	{
		m_Backend->WaitForGPUIdle();
	}

	void Renderer::Render()
	{
		m_Backend->BeginRender();

		m_Backend->DepthPass();
		m_Backend->GeometryPass();
		m_Backend->SSAOPass();
		m_Backend->ShadowPass();
		m_Backend->LightingPass();
		m_Backend->TonemappingPass();
		m_Backend->AntialiasPass();
		m_Backend->ImGuiPass();

		m_Backend->EndRender();
	}

	void Renderer::SetMainCamera(Camera* camera)
	{
		m_Backend->SetMainCamera(camera);
	}
	Camera* Renderer::GetMainCamera()
	{
		return m_Backend->GetMainCamera();
	}

	void Renderer::SetVSync(bool vSync)
	{
		m_Backend->SetVSync(vSync);
	}

	void Renderer::SetShadowCascadesWeight(float weight)
	{
		m_Backend->SetShadowCascadesWeight(weight);
	}
	const float Renderer::GetShadowCascadesWeight() const
	{
		return m_Backend->GetShadowCascadesWeight();
	}

	void Renderer::SetShadowCascadesFarPlane(float farPlane)
	{
		m_Backend->SetShadowCascadesFarPlane(farPlane);
	}
	const float Renderer::GetShadowCascadesFarPlane() const
	{
		return m_Backend->GetShadowCascadesFarPlane();
	}

	void Renderer::SetPointShadowResolution(uint32_t resolution)
	{
		m_Backend->SetPointShadowResolution(resolution);
	}
	const float Renderer::GetPointShadowResolution() const
	{
		return m_Backend->GetPointShadowResolution();
	}

	void Renderer::SetSpotShadowResolution(uint32_t resolution)
	{
		m_Backend->SetSpotShadowResolution(resolution);
	}
	const float Renderer::GetSpotShadowResolution() const
	{
		return m_Backend->GetSpotShadowResolution();
	}

	void Renderer::SetDirShadowResolution(uint32_t resolution)
	{
		m_Backend->SetDirShadowResolution(resolution);
	}
	const float Renderer::GetDirShadowResolution() const
	{
		return m_Backend->GetDirShadowResolution();
	}

	void Renderer::SetShadowFormat(VkFormat format)
	{
		m_Backend->SetShadowFormat(format);
	}
	const VkFormat Renderer::GetShadowFormat() const
	{
		return m_Backend->GetShadowFormat();
	}

	Renderer* Renderer::Get()
	{
		if (!g_CurrentRenderer)
			EN_ERROR("Renderer::GetRenderer() - g_CurrentRenderer was a nullptr!");

		return g_CurrentRenderer;
	}

	void Renderer::ReloadRenderer()
	{
		m_Backend->ReloadBackend();
	}
	RendererBackend::PostProcessingParams& Renderer::GetPPParams()
	{
		return m_Backend->m_PostProcessParams;
	}
	void Renderer::SetUIRenderCallback(std::function<void()> callback)
	{
		m_Backend->m_ImGuiRenderCallback = callback;
	}
	void Renderer::SetDebugMode(int mode)
	{
		m_Backend->m_DebugMode = mode;
	}
}