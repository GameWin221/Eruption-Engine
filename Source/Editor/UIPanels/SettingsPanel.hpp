#pragma once

#ifndef EN_SETTINGSPANEL_HPP
#define EN_SETTINGSPANEL_HPP

#include <Renderer/Renderer.hpp>

namespace en
{
	class SettingsPanel
	{
	public:
		SettingsPanel(Renderer* renderer);

		void Render();

	private:
		Renderer* m_Renderer;
	};
}

#endif