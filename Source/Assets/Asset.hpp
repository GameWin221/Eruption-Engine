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

	struct Asset
	{
		AssetType m_Type = AssetType::None;
	};
}

#endif