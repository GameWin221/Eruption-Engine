#include "Core/EnPch.hpp"
#include "SettingsPanel.hpp"

#include <Editor/EditorCommons.hpp>

namespace en
{
	constexpr std::array<const char*, 2> g_AntialiasingModeNames = { "FXAA", "None" };

	void SettingsPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

		ImGui::Begin("Settings", nullptr, EditorCommons::CommonFlags);

		auto& postProcessing = m_Renderer->GetPPParams();

		static bool vSync = true;

		if (ImGui::Checkbox("VSync", &vSync))
			m_Renderer->SetVSync(vSync);

		if (ImGui::CollapsingHeader("Antialiasing"))
		{
			if (ImGui::Combo("Antialiasing Mode", (int*)&postProcessing.antialiasingMode, g_AntialiasingModeNames.data(), g_AntialiasingModeNames.size()))
				m_Renderer->ReloadRenderer();
			
			switch (postProcessing.antialiasingMode)
			{
			case RendererBackend::AntialiasingMode::FXAA:
				ImGui::Spacing();

				if (ImGui::Button("Restore Defaults"))
					postProcessing.antialiasing = RendererBackend::PostProcessingParams::Antialiasing{};

				ImGui::Spacing();

				ImGui::DragFloat("FXAA Span Max", &postProcessing.antialiasing.fxaaSpanMax, 0.02f, 0.0f, 16.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Min", &postProcessing.antialiasing.fxaaReduceMin, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Mult", &postProcessing.antialiasing.fxaaReduceMult, 0.01f, 0.0f, 1.0f), "%.3f", ImGuiSliderFlags_AlwaysClamp;
				ImGui::DragFloat("FXAA Power", &postProcessing.antialiasing.fxaaPower, 0.2f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				break;

			case RendererBackend::AntialiasingMode::None:
				ImGui::Text("Antialiasing is disabled.");
				break;

			default:
				EN_ERROR("void SettingsPanel::Render() - postProcessing.antialiasingMode was an unknown value!");
				break;
			}
	
		}

		ImGui::End();
	}
}