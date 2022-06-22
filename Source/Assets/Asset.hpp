#pragma once

#ifndef EN_ASSET_HPP
#define EN_ASSET_HPP

namespace en
{
	enum struct AssetType
	{
		Mesh,
		SubMesh,
		Texture,
		Material,
		None
	};

	struct Asset;

	template<class T>
	concept IsAssetChild = std::derived_from<T, Asset> || std::same_as<T, Asset>;

	struct Asset
	{
		const AssetType m_Type = AssetType::None;

		template<IsAssetChild T>
		T* CastTo()
		{
			return reinterpret_cast<T*>(this);
		}
	};
}

#endif