#pragma once

#ifndef EN_ASSETMANAGER_HPP
#define EN_ASSETMANAGER_HPP

#include <Assets/Texture.hpp>
#include <Assets/Mesh.hpp>
#include <Assets/Model.hpp>

namespace en
{
	class AssetManager
	{
	public:
		void LoadModel	(std::string nameID, std::string path, std::string defaultTexture = "WHITE_TEXTURE");
		void LoadTexture(std::string nameID, std::string path, bool loadFlipped = true);

		void UnloadModel  (std::string nameID);
		void UnloadTexture(std::string nameID);

		Model*   GetModel  (std::string nameID);
		Texture* GetTexture(std::string nameID);

	private:
		std::unordered_map<std::string, std::unique_ptr<Model  >> m_Models;
		std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
	};
}


#endif