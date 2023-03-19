#include "LightsBuffer.hpp"

namespace en
{
	LightsBuffer::LightsBuffer()
	{
		m_Buffer = MakeHandle<MemoryBuffer>(
			sizeof(LightsBufferObject),
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		for (auto& buffer : m_StagingBuffers)
			buffer = MakeHandle<MemoryBuffer>(
				sizeof(LightsBufferObject),
				VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VMA_MEMORY_USAGE_CPU_TO_GPU
			);
	}

	void LightsBuffer::MapStagingMemory(uint32_t frameIndex)
	{
		m_StagingBuffers[frameIndex]->MapMemory(&LBO, m_StagingBuffers[frameIndex]->GetSize());
	}
	void LightsBuffer::CopyStagingToDevice(uint32_t frameIndex, VkCommandBuffer cmd)
	{
		m_StagingBuffers[frameIndex]->CopyTo(m_Buffer, m_Buffer->GetSize(), 0U, 0U, cmd);

		m_Buffer->PipelineBarrier(
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			cmd
		);
	}
}