#include <Core/EnPch.hpp>
#include "IndexBuffer.hpp"

namespace en
{
	IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices) : m_IndicesCount(indices.size())
	{
		const VkDeviceSize bufferSize = m_IndicesCount * sizeof(indices[0]);

		MemoryBuffer stagingBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(indices.data(), bufferSize);


		m_Buffer = std::make_unique<MemoryBuffer>(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		
		stagingBuffer.CopyTo(m_Buffer.get());
	}

	void IndexBuffer::Bind(VkCommandBuffer& commandBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_Buffer->GetHandle(), 0, VK_INDEX_TYPE_UINT32);
	}
}