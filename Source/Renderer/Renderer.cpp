#include <Core/EnPch.hpp>
#include "Renderer.hpp"

namespace en
{
	Renderer* g_CurrentRenderer;

	Renderer::Renderer(RendererInfo& rendererInfo)
	{
		g_CurrentRenderer = this;

		m_Backend.Init(rendererInfo);

		EN_SUCCESS("Created the renderer");
	}
	Renderer::~Renderer(){}

	void Renderer::BindScene(Scene* scene)
	{
		m_Backend.BindScene(scene);
	}
	void Renderer::UnbindScene()
	{
		m_Backend.UnbindScene();
	}

	Scene* Renderer::GetScene()
	{
		return m_Backend.GetScene();
	}

	void Renderer::WaitForGPUIdle()
	{
		m_Backend.WaitForGPUIdle();
	}

	void Renderer::Render()
	{
		m_Backend.BeginRender();

		m_Backend.GeometryPass();
		m_Backend.LightingPass();
		m_Backend.PostProcessPass();
		m_Backend.ImGuiPass();

		m_Backend.EndRender();
	}

	void Renderer::SetMainCamera(Camera* camera)
	{
		m_Backend.SetMainCamera(camera);
	}
	Camera* Renderer::GetMainCamera()
	{
		return m_Backend.GetMainCamera();
	}

	Renderer& Renderer::GetRenderer()
	{
		if (!g_CurrentRenderer)
			EN_ERROR("Renderer::GetRenderer() - g_CurrentRenderer was a nullptr!");

		return *g_CurrentRenderer;
	}

	void Renderer::ReloadRenderer()
	{
		m_Backend.ReloadBackend();
	}
	void Renderer::SetUIRenderCallback(std::function<void()> callback)
	{
		m_Backend.m_ImGuiRenderCallback = callback;
	}
	void Renderer::SetDebugMode(int& mode)
	{
		m_Backend.m_DebugMode = mode;
	}

	std::array<PointLight, MAX_LIGHTS>& Renderer::GetPointLights()
	{
		return m_Backend.GetPointLights();
	}
}