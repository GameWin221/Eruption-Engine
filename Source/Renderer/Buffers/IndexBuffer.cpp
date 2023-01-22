#include "IndexBuffer.hpp"

namespace en
{
	IndexBuffer::IndexBuffer(const std::vector<uint32_t>& indices) : m_IndicesCount(indices.size())
	{
		VkDeviceSize bufferSize = m_IndicesCount * sizeof(uint32_t);

		MemoryBuffer stagingBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VMA_MEMORY_USAGE_GPU_TO_CPU
		);

		stagingBuffer.MapMemory(indices.data(), bufferSize);

		m_Buffer = MakeHandle<MemoryBuffer>(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);
		
		stagingBuffer.CopyTo(m_Buffer);
	}
}