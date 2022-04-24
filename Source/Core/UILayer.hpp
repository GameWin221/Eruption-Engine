#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Renderer/Renderer.hpp>

namespace en
{
	class UILayer
	{
	public:
		void AttachTo(Renderer* renderer, double* deltaTimeVar);

		void OnUIDraw();

	private:
		Renderer* m_Renderer;
		double* m_DeltaTime;

		void DrawLightsMenu();
		void DrawDebugMenu();
		void DrawCameraMenu();
	};
}

#endif