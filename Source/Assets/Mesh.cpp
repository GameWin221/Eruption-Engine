#include "Mesh.hpp"

namespace en
{
	Handle<Mesh> g_EmptyMesh;

	Handle<Mesh> Mesh::GetEmptyMesh()
	{
		if (!g_EmptyMesh)
		{
			g_EmptyMesh = MakeHandle<Mesh>();
			g_EmptyMesh->m_FilePath = "No Mesh";
			g_EmptyMesh->m_Name = "No Mesh";
		}

		return g_EmptyMesh;
	}
}