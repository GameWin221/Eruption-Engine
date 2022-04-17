#include <Core/EnPch.hpp>
#include "VertexBuffer.hpp"

namespace en
{
	VertexBuffer::VertexBuffer(const std::vector<Vertex>& vertexData)
	{
		m_Size = vertexData.size();

		VkDeviceSize bufferSize = m_Size * sizeof(Vertex);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		en::Helpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		en::Helpers::MapBuffer(stagingBufferMemory, vertexData.data(), bufferSize);

		en::Helpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory);
		en::Helpers::CopyBuffer(stagingBuffer, m_Buffer, bufferSize);

		en::Helpers::DestroyBuffer(stagingBuffer, stagingBufferMemory);
	}
	VertexBuffer::~VertexBuffer()
	{
		en::Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}

	void VertexBuffer::Bind(VkCommandBuffer& commandBuffer)
	{
		const VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m_Buffer, &offset);
	}
}