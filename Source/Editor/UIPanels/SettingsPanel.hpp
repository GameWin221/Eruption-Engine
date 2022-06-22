#pragma once

#ifndef EN_SETTINGSPANEL_HPP
#define EN_SETTINGSPANEL_HPP

#include <Renderer/Renderer.hpp>

namespace en
{
	class SettingsPanel
	{
	public:
		constexpr SettingsPanel(Renderer* renderer) : m_Renderer(renderer) {};

		void Render();

	private:
		Renderer* m_Renderer;
	};
}

#endif