#pragma once

#ifndef EN_EDITORCOMMONS
#define EN_EDITORCOMMONS

#include <imgui.h>

#define SPACE(); {ImGui::Spacing();ImGui::Separator();ImGui::Spacing();}

namespace en
{
	namespace EditorCommons
	{
		static constexpr ImGuiWindowFlags CommonFlags = ImGuiWindowFlags_None | ImGuiWindowFlags_NoCollapse;

		static constexpr ImVec2 FreeWindowMinSize = ImVec2(500 , 190 );
		static constexpr ImVec2 FreeWindowMaxSize = ImVec2(1920, 1080);

		static const void TextCentered(std::string text)
		{
			float fontSize = ImGui::GetFontSize() * text.size() / 2;

			ImGui::SameLine(ImGui::GetWindowSize().x / 2 - fontSize + (fontSize / 2));

			ImGui::Text(text.c_str());
		}
	}
}

#endif