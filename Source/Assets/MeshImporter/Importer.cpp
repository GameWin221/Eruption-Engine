#include "Importer.hpp"

namespace en
{
	static uint32_t s_MaterialCounter = 0U;

	Importer::Importer(const MeshImportProperties& properties, Handle<Material> defaultMaterial, Handle<Texture> defaultSRGBTexture, Handle<Texture> defaultNonSRGBTexture) 
		: m_ImportProperties(properties), m_DefaultMaterial(defaultMaterial), m_DefaultSRGBTexture(defaultSRGBTexture), m_DefaultNonSRGBTexture(defaultNonSRGBTexture)
	{
	}
	uint32_t& Importer::GetMaterialCounter()
	{
		return s_MaterialCounter;
	}
}