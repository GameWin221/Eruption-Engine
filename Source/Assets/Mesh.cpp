#include "Mesh.hpp"

namespace en
{
	Handle<Mesh> g_EmptyMesh;

	Handle<Mesh> Mesh::GetEmptyMesh()
	{
		if (!g_EmptyMesh)
			g_EmptyMesh = MakeHandle<Mesh>("No Mesh", "No Mesh");

		return g_EmptyMesh;
	}
}