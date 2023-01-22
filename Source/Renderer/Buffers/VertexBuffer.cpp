#include "VertexBuffer.hpp"

namespace en
{
	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices)
		: m_VerticesCount(vertices.size())
	{
		VkDeviceSize bufferSize = vertices.size() * sizeof(Vertex);

		MemoryBuffer stagingBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
		stagingBuffer.MapMemory(vertices.data(), bufferSize);

		m_Buffer = MakeHandle<MemoryBuffer>(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		stagingBuffer.CopyTo(m_Buffer);
	}
}