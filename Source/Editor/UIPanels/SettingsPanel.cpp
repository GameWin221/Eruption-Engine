#include "SettingsPanel.hpp"

#include <Editor/EditorCommons.hpp>

namespace en
{
	constexpr std::array<const char*, 2> g_AntialiasingModeNames = { "None", "FXAA"};
	constexpr std::array<const char*, 2> g_AmbientOcclusionModeNames = { "None", "SSAO" };
	constexpr std::array<const char*, 4> g_QualityLevelNames = { "Low", "Medium", "High", "Ultra"};

	void SettingsPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

		ImGui::Begin("Settings", nullptr, EditorCommons::CommonFlags);

		static bool vSync = m_Renderer->GetVSyncEnabled();
		static bool depthPrepass = m_Renderer->GetDepthPrepassEnabled();

		static Renderer::AntialiasingMode antialiasingMode = m_Renderer->GetAntialiasingMode();
		static Renderer::AmbientOcclusionMode ssaoMode = m_Renderer->GetAmbientOcclusionMode();

		//static Renderer::QualityLevel antialiasingQuality = m_Renderer->GetAntialaliasingQuality();
		static Renderer::QualityLevel aoModeQuality = m_Renderer->GetAmbientOcclusionQuality();

		if (ImGui::Checkbox("VSync", &vSync))
			m_Renderer->SetVSyncEnabled(vSync);

		if (ImGui::Checkbox("Depth Prepass", &depthPrepass))
			m_Renderer->SetDepthPrepassEnabled(depthPrepass);
		
		if (ImGui::CollapsingHeader("Antialiasing"))
		{
			if (ImGui::Combo("Antialiasing Mode", (int*)&antialiasingMode, g_AntialiasingModeNames.data(), g_AntialiasingModeNames.size()))
				m_Renderer->SetAntialiasingMode(antialiasingMode);

			//if (ImGui::Combo("Antialiasing Quality", (int*)&antialiasingQuality, g_QualityLevelNames.data(), g_QualityLevelNames.size()))
			//	m_Renderer->SetAntialaliasingQuality(antialiasingQuality);
			
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
		
		if (ImGui::CollapsingHeader("Ambient Occlusion"))
		{
			if (ImGui::Combo("Ambient Occlusion Mode", (int*)&ssaoMode, g_AmbientOcclusionModeNames.data(), g_AmbientOcclusionModeNames.size()))
				m_Renderer->SetAmbientOcclusionMode(ssaoMode);

			if (ImGui::Combo("Ambient Occlusion Quality", (int*)&aoModeQuality, g_QualityLevelNames.data(), g_QualityLevelNames.size()))
				m_Renderer->SetAmbientOcclusionQuality(aoModeQuality);

			auto& ssao = m_Renderer->GetAmbientOcclusionProperties();
			int s = ssao._samples;
			switch (ssaoMode)
			{
			case Renderer::AmbientOcclusionMode::SSAO:
				ImGui::Spacing();

				if (ImGui::Button("Restore Defaults"))
					ssao = Renderer::AmbientOcclusionProperties{};

				ImGui::Spacing();
				
				ImGui::DragFloat("SSAO Bias", &ssao.bias, 0.05f, 0.0f, 1.0f), "%.2f", ImGuiSliderFlags_AlwaysClamp;
				ImGui::DragFloat("SSAO Radius", &ssao.radius, 0.1f, 0.0f, 5.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("SSAO Multiplier", &ssao.multiplier, 0.1f, 0.0f, 5.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("ssao_noise_scale", &ssao._noiseScale, 0.1f, 1.0f, 8.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragInt("ssao_samples", &s, 1.0f, 1.0f, 64.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp);
				
				break;

			case Renderer::AmbientOcclusionMode::None:
				ImGui::Text("Ambient occlusion is disabled.");
				break;

			default:
				EN_ERROR("void SettingsPanel::Render() - postProcessing.ambientOcclusionMode was an unknown value!");
				break;
			}
			ssao._samples = s;
		}

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