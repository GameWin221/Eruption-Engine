#include <Core/EnPch.hpp>
#include "Mesh.hpp"

namespace en
{
	Mesh* g_EmptyMesh;

	Mesh* Mesh::GetEmptyMesh()
	{
		if (!g_EmptyMesh)
		{
			g_EmptyMesh = new Mesh();
			g_EmptyMesh->m_FilePath = "No Mesh";
			g_EmptyMesh->m_Name = "No Mesh";
		}

		return g_EmptyMesh;
	}
}