#pragma once

#ifndef EN_SCENEMANAGER_HPP
#define EN_SCENEMANAGER_HPP

#include <Scene/Scene.hpp>

namespace en
{
	class SceneManager
	{
	public:
		//void Save();
		//void Load();

	private:
		std::unordered_map<std::string, Scene> m_Scenes;
	};
}
#endif