#include <Core/EnPch.hpp>
#include "IndexBuffer.hpp"

namespace en
{
	IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indexData)
	{
		UseContext();

		m_Size = indexData.size();

		VkDeviceSize bufferSize = m_Size * sizeof(uint32_t);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		en::Helpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		en::Helpers::MapBuffer(stagingBufferMemory, indexData.data(), bufferSize);

		en::Helpers::CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_Buffer, m_BufferMemory);
		en::Helpers::CopyBuffer(stagingBuffer, m_Buffer, bufferSize);

		en::Helpers::DestroyBuffer(stagingBuffer, stagingBufferMemory);
	}
	IndexBuffer::~IndexBuffer()
	{
		en::Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}

	void IndexBuffer::Bind(VkCommandBuffer& commandBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_Buffer, 0, VK_INDEX_TYPE_UINT32);
	}
}