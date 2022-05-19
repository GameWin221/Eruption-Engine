#include <Core/EnPch.hpp>
#include "VertexBuffer.hpp"

namespace en
{
	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertices) : m_VerticesCount(vertices.size())
	{
		const VkDeviceSize bufferSize = m_VerticesCount * sizeof(vertices[0]);

		MemoryBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(vertices.data(), bufferSize);


		m_Buffer = std::make_unique<MemoryBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		stagingBuffer.CopyTo(m_Buffer.get());
	}

	constexpr VkDeviceSize vertexBufferZeroOffset = 0;

	void VertexBuffer::Bind(VkCommandBuffer& commandBuffer)
	{
		vkCmdBindVertexBuffers(commandBuffer, 0U, 1U, &m_Buffer->GetHandle(), &vertexBufferZeroOffset);
	}
}