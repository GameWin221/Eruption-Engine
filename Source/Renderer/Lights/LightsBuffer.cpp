#include "Core/EnPch.hpp"
#include "LightsBuffer.hpp"

namespace en
{
	LightsBuffer::LightsBuffer()
	{
		m_Buffer = std::make_unique<MemoryBuffer>(
			sizeof(LightsBufferObject),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		for (auto& buffer : m_StagingBuffers)
			buffer = std::make_unique<MemoryBuffer>(
				sizeof(LightsBufferObject),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
	}

	void LightsBuffer::MapStagingMemory(uint32_t frameIndex)
	{
		m_StagingBuffers[frameIndex]->MapMemory(&LBO, m_StagingBuffers[frameIndex]->m_BufferSize);
	}
	void LightsBuffer::CopyStagingToDevice(uint32_t frameIndex, VkCommandBuffer cmd)
	{
		m_StagingBuffers[frameIndex]->CopyTo(m_Buffer.get(), cmd);

		m_Buffer->PipelineBarrier(
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			cmd
		);
	}
}