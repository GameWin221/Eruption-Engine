#include "Core/EnPch.hpp"
#include "CameraBuffer.hpp"

namespace en
{
	CameraBuffer::CameraBuffer()
	{
		for (auto& buffer : m_Buffers)
		{
			buffer = std::make_unique<MemoryBuffer>(
				sizeof(CameraBufferObject),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		}


		for (uint32_t i = 0U; i < FRAMES_IN_FLIGHT; i++)
		{
			m_DescriptorSets[i] = std::make_unique<DescriptorSet>(
				std::vector<DescriptorSet::ImageInfo>{},
				std::vector<DescriptorSet::BufferInfo>{{
						.index = 0U,
						.buffer = m_Buffers[i]->m_Handle,
						.size = m_Buffers[i]->m_BufferSize,

						.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,

						.stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
					}}
			);
		}
		
	}

	void CameraBuffer::MapBuffer(uint32_t frameIndex)
	{
		m_Buffers[frameIndex]->MapMemory(&m_CBOs[frameIndex], m_Buffers[frameIndex]->m_BufferSize);
	}

	void CameraBuffer::UpdateBuffer(uint32_t frameIndex, Camera* camera, VkExtent2D extent, int debugMode)
	{
		m_CBOs[frameIndex].debugMode = debugMode;

		m_CBOs[frameIndex].position = camera->m_Position;

		m_CBOs[frameIndex].proj = camera->GetProjMatrix();
		m_CBOs[frameIndex].view = camera->GetViewMatrix();
		m_CBOs[frameIndex].invView = glm::inverse(m_CBOs[frameIndex].view);
		m_CBOs[frameIndex].projView = m_CBOs[frameIndex].proj * m_CBOs[frameIndex].view;

		m_CBOs[frameIndex].zNear = camera->m_NearPlane;
		m_CBOs[frameIndex].zFar = camera->m_FarPlane;

		uint32_t sizeX = (uint32_t)std::ceilf((float)extent.width / CLUSTERED_TILES_X);
		uint32_t sizeY = (uint32_t)std::ceilf((float)extent.height / CLUSTERED_TILES_Y);

		m_CBOs[frameIndex].clusterTileSizes = glm::uvec4(sizeX, sizeY, 0, 0);
		m_CBOs[frameIndex].clusterTileCount = glm::uvec4(CLUSTERED_TILES_X, CLUSTERED_TILES_Y, CLUSTERED_TILES_Z, 0U);

		m_CBOs[frameIndex].clusterScale = (float)CLUSTERED_TILES_Z / std::log2f(camera->m_FarPlane / camera->m_NearPlane);
		m_CBOs[frameIndex].clusterBias = -((float)CLUSTERED_TILES_Z * std::log2f(camera->m_NearPlane) / std::log2f(camera->m_FarPlane / camera->m_NearPlane));

	}

	VkDescriptorSetLayout CameraBuffer::GetLayout()
	{
		return m_DescriptorSets[0]->m_DescriptorLayout;
	}
	DescriptorSet* CameraBuffer::GetDescriptorHandle(uint32_t frameIndex)
	{
		return m_DescriptorSets[frameIndex].get();
	}
}