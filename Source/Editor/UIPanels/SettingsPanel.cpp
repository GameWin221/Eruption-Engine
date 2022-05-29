#include "Core/EnPch.hpp"
#include "SettingsPanel.hpp"

#include <Editor/EditorCommons.hpp>

namespace en
{
	SettingsPanel::SettingsPanel(Renderer* renderer) : m_Renderer(renderer) {}

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
			static const std::vector<const char*> modes = { "FXAA", "None"};

			if (ImGui::Combo("Antialiasing Mode", (int*)&postProcessing.antialiasingMode, modes.data(), modes.size()))
				m_Renderer->ReloadRenderer();
			
			if (postProcessing.antialiasingMode == VulkanRendererBackend::AntialiasingMode::FXAA)
			{
				ImGui::Spacing();

				if (ImGui::Button("Restore Defaults"))
					postProcessing.antialiasing = VulkanRendererBackend::PostProcessingParams::Antialiasing{};

				ImGui::Spacing();

				ImGui::DragFloat("FXAA Span Max", &postProcessing.antialiasing.fxaaSpanMax, 0.02f, 0.0f, 16.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Min", &postProcessing.antialiasing.fxaaReduceMin, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
				ImGui::DragFloat("FXAA Reduce Mult", &postProcessing.antialiasing.fxaaReduceMult, 0.01f, 0.0f, 1.0f), "%.3f", ImGuiSliderFlags_AlwaysClamp;
				ImGui::DragFloat("FXAA Power", &postProcessing.antialiasing.fxaaPower, 0.2f, 0.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			}
			//else if (postProcessing.antialiasingMode == VulkanRendererBackend::AntialiasingMode::SMAA)
			//{
			//	ImGui::Text("This antialiasing mode doesn't work yet!");
			//}
			else if (postProcessing.antialiasingMode == VulkanRendererBackend::AntialiasingMode::None)
				ImGui::Text("Antialiasing is disabled.");
			
		}

		ImGui::End();
	}
}