
#include "CSMBuffer.hpp"

namespace en
{
    CSMBuffer::CSMBuffer()
    {
		for (auto& buffer : m_Buffers)
		{
			buffer = MakeHandle<MemoryBuffer>(
				sizeof(CSMBufferObject),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY
			);
		}


		for (uint32_t i = 0U; i < FRAMES_IN_FLIGHT; i++)
		{
			m_DescriptorSets[i] = MakeHandle<DescriptorSet>(
				std::vector<DescriptorSet::ImageInfo>{},
				std::vector<DescriptorSet::BufferInfo>{{
						.index = 0U,
						.buffer = m_Buffers[i]->GetHandle(),
						.size = m_Buffers[i]->m_BufferSize,

						.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,

						.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
					}}
			);
		}
    }
    void CSMBuffer::MapBuffer(uint32_t frameIndex)
    {
		m_Buffers[frameIndex]->MapMemory(&m_CSMBOs[frameIndex], m_Buffers[frameIndex]->m_BufferSize);
    }

	VkDescriptorSetLayout CSMBuffer::GetLayout()
	{
		return m_DescriptorSets[0]->GetLayout();
	}
	Handle<DescriptorSet> CSMBuffer::GetDescriptorHandle(uint32_t frameIndex)
	{
		return m_DescriptorSets[frameIndex];
	}
}