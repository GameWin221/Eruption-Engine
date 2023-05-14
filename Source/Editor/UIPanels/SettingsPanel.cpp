#include "SettingsPanel.hpp"

#include <Editor/EditorCommons.hpp>

namespace en
{
	constexpr std::array<const char*, 2> g_AntialiasingModeNames = { "None", "FXAA"};
	constexpr std::array<const char*, 2> g_AmbientOcclusionModeNames = { "SSAO", "None" };

	void SettingsPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

		ImGui::Begin("Settings", nullptr, EditorCommons::CommonFlags);

		static bool vSync = m_Renderer->GetVSyncEnabled();
		static bool depthPrepass = m_Renderer->GetDepthPrepassEnabled();
		static Renderer::AntialiasingMode antialiasingMode = m_Renderer->GetAntialiasingMode();

		if (ImGui::Checkbox("VSync", &vSync))
			m_Renderer->SetVSyncEnabled(vSync);

		if (ImGui::Checkbox("Depth Prepass", &depthPrepass))
			m_Renderer->SetDepthPrepassEnabled(depthPrepass);
		
		if (ImGui::CollapsingHeader("Antialiasing"))
		{
			if (ImGui::Combo("Antialiasing Mode", (int*)&antialiasingMode, g_AntialiasingModeNames.data(), g_AntialiasingModeNames.size()))
				m_Renderer->SetAntialiasingMode(antialiasingMode);
			
			auto& aa = m_Renderer->GetAntialiasingProperties();

			switch (m_Renderer->GetAntialiasingMode())
			{
			case Renderer::AntialiasingMode::FXAA:
				ImGui::Spacing();

				if (ImGui::Button("Restore Defaults"))
					m_Renderer->GetAntialiasingProperties() = Renderer::AntialiasingProperties{};

				ImGui::Spacing();

				ImGui::DragFloat("FXAA Span Max", &aa.fxaaSpanMax, 0.02f, 0.0f, 16.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Min", &aa.fxaaReduceMin, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Mult", &aa.fxaaReduceMult, 0.01f, 0.0f, 1.0f), "%.3f", ImGuiSliderFlags_AlwaysClamp;
				ImGui::DragFloat("FXAA Power", &aa.fxaaPower, 0.2f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				break;

			case Renderer::AntialiasingMode::None:
				ImGui::Text("Antialiasing is disabled.");
				break;

			default:
				EN_ERROR("void SettingsPanel::Render() - postProcessing.antialiasingMode was an unknown value!");
				break;
			}
	
		}
		/*
		if (ImGui::CollapsingHeader("Ambient Occlusion"))
		{
			if (ImGui::Combo("Ambient Occlusion Mode", (int*)&m_Renderer->m_PostProcessParams.ambientOcclusionMode, g_AmbientOcclusionModeNames.data(), g_AmbientOcclusionModeNames.size()))
				m_Renderer->ReloadBackend();

			switch (m_Renderer->m_PostProcessParams.ambientOcclusionMode)
			{
			case Renderer::AmbientOcclusionMode::SSAO:
				ImGui::Spacing();

				if (ImGui::Button("Restore Defaults"))
					m_Renderer->m_PostProcessParams.ambientOcclusion = Renderer::PostProcessingParams::AmbientOcclusion{};

				ImGui::Spacing();

				ImGui::DragFloat("SSAO Bias", &m_Renderer->m_PostProcessParams.ambientOcclusion.bias, 0.05f, 0.0f, 1.0f), "%.2f", ImGuiSliderFlags_AlwaysClamp;
				ImGui::DragFloat("SSAO Radius", &m_Renderer->m_PostProcessParams.ambientOcclusion.radius, 0.1f, 0.0f, 5.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("SSAO Multiplier", &m_Renderer->m_PostProcessParams.ambientOcclusion.multiplier, 0.1f, 0.0f, 5.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				break;

			case Renderer::AmbientOcclusionMode::None:
				ImGui::Text("Ambient occlusion is disabled.");
				break;

			default:
				EN_ERROR("void SettingsPanel::Render() - postProcessing.ambientOcclusionMode was an unknown value!");
				break;
			}

		}
		*/

		if (ImGui::CollapsingHeader("Lights and Shadows"))
		{
			static float weight = m_Renderer->GetShadowCascadesWeight();
			if (ImGui::DragFloat("Shadow cascades split weight", &weight, 0.01f, 0.05f, 0.95f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
				m_Renderer->SetShadowCascadesWeight(weight);

			static float farPlane = m_Renderer->GetShadowCascadesFarPlane();
			if (m_Renderer->GetScene())
				if (ImGui::DragFloat("Shadow cascades far plane", &farPlane, 0.5f, m_Renderer->GetScene()->m_MainCamera->m_NearPlane, m_Renderer->GetScene()->m_MainCamera->m_FarPlane, "%.2f", ImGuiSliderFlags_AlwaysClamp))
					m_Renderer->SetShadowCascadesFarPlane(farPlane);

			static int pRes = m_Renderer->GetPointShadowResolution();
			static int sRes = m_Renderer->GetSpotShadowResolution();
			static int dRes = m_Renderer->GetDirShadowResolution();

			if (ImGui::DragInt("Point shadows resolution", &pRes, 2, 16, 8192, "%d", ImGuiSliderFlags_AlwaysClamp))
				m_Renderer->SetPointShadowResolution(pRes);

			if (ImGui::DragInt("Spot shadows resolution", &sRes, 2, 16, 8192, "%d", ImGuiSliderFlags_AlwaysClamp))
				m_Renderer->SetSpotShadowResolution(sRes);

			if (ImGui::DragInt("Directional shadows resolution", &dRes, 2, 16, 8192, "%d", ImGuiSliderFlags_AlwaysClamp))
				m_Renderer->SetDirShadowResolution(dRes);

			bool enabled32BitShadows = (m_Renderer->GetShadowFormat() == VK_FORMAT_D32_SFLOAT);

			if (ImGui::Checkbox("32 Bit Shadowmaps", &enabled32BitShadows))
			{
				if (enabled32BitShadows)
					m_Renderer->SetShadowFormat(VK_FORMAT_D32_SFLOAT);
				else
					m_Renderer->SetShadowFormat(VK_FORMAT_D16_UNORM);
			}
		}
	
		ImGui::End();
	}
}