#include "EnPch.hpp"
#include "Rendering/Renderer.hpp"

namespace en
{
	Renderer* g_CurrentRenderer;

	Renderer::Renderer(RendererInfo& rendererInfo)
	{
		g_CurrentRenderer = this;

		m_Backend.Init(rendererInfo);
	}
	Renderer::~Renderer(){}

	void Renderer::PrepareModel(Model* model)
	{
		m_Backend.PrepareModel(model);
	}
	void Renderer::RemoveModel(Model* model)
	{
		m_Backend.RemoveModel(model);
	}
	void Renderer::EnqueueModel(Model* model)
	{
		m_Backend.EnqueueModel(model);
	}

	void Renderer::Render()
	{
		m_Backend.BeginRender();

		m_Backend.GeometryPass();
		m_Backend.LightingPass();

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
		return *g_CurrentRenderer;
	}
}