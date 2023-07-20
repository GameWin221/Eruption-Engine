
#include "CameraBuffer.hpp"

namespace en
{
	CameraBuffer::CameraBuffer()
	{
		for (auto& buffer : m_Buffers)
		{
			buffer = MakeHandle<MemoryBuffer>(
				sizeof(CameraBufferObject),
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VMA_MEMORY_USAGE_CPU_ONLY
			);
		}

		for (uint32_t i = 0U; i < FRAMES_IN_FLIGHT; i++)
		{
			m_DescriptorSets[i] = MakeHandle<DescriptorSet>(DescriptorInfo{
				std::vector<DescriptorInfo::ImageInfo>{},
				std::vector<DescriptorInfo::BufferInfo>{ {
					.index  = 0U,
					.buffer = m_Buffers[i]->GetHandle(),
					.size = m_Buffers[i]->GetSize(),
					.type   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stage  = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
				}}}
			);
		}
	}

	void CameraBuffer::MapBuffer(uint32_t frameIndex)
	{
		m_Buffers[frameIndex]->MapMemory(&m_CBOs[frameIndex], m_Buffers[frameIndex]->GetSize());
	}

	void CameraBuffer::UpdateBuffer(
		uint32_t frameIndex,
		Handle<Camera> camera,
		std::array<float, SHADOW_CASCADES>& cascadeSplitDistances,
		std::array<float, SHADOW_CASCADES>& cascadeFrustumSizeRatios,
		std::array<std::array<glm::mat4, SHADOW_CASCADES>, MAX_DIR_LIGHT_SHADOWS>& cascadeMatrices,
		VkExtent2D extent,
		int debugMode
	) {
		m_CBOs[frameIndex].debugMode = debugMode;

		m_CBOs[frameIndex].position = camera->m_Position;

		m_CBOs[frameIndex].proj = camera->GetProjMatrix();
		m_CBOs[frameIndex].invProj = glm::inverse(m_CBOs[frameIndex].proj);

		m_CBOs[frameIndex].view = camera->GetViewMatrix();
		m_CBOs[frameIndex].invView = glm::inverse(m_CBOs[frameIndex].view);

		m_CBOs[frameIndex].projView = m_CBOs[frameIndex].proj * m_CBOs[frameIndex].view;
		m_CBOs[frameIndex].invProjView = glm::inverse(m_CBOs[frameIndex].projView);

		m_CBOs[frameIndex].zNear = camera->m_NearPlane;
		m_CBOs[frameIndex].zFar = camera->m_FarPlane;

		for (uint32_t i = 0U; i < SHADOW_CASCADES; i++)
		{
			m_CBOs[frameIndex].cascadeSplitDistances[i].x = cascadeSplitDistances[i];
			m_CBOs[frameIndex].cascadeFrustumSizeRatios[i].x = cascadeFrustumSizeRatios[i];

			for (uint32_t j = 0U; j < MAX_DIR_LIGHT_SHADOWS; j++)
				m_CBOs[frameIndex].cascadeMatrices[j][i] = cascadeMatrices[j][i];
		}

		uint32_t sizeX = (uint32_t)std::ceilf((float)extent.width / CLUSTERED_TILES_X);
		uint32_t sizeY = (uint32_t)std::ceilf((float)extent.height / CLUSTERED_TILES_Y);

		m_CBOs[frameIndex].clusterTileSizes = glm::uvec4(sizeX, sizeY, 0, 0);
		m_CBOs[frameIndex].clusterTileCount = glm::uvec4(CLUSTERED_TILES_X, CLUSTERED_TILES_Y, CLUSTERED_TILES_Z, 0U);

		m_CBOs[frameIndex].clusterScale = (float)CLUSTERED_TILES_Z / std::log2f(camera->m_FarPlane / camera->m_NearPlane);
		m_CBOs[frameIndex].clusterBias = -((float)CLUSTERED_TILES_Z * std::log2f(camera->m_NearPlane) / std::log2f(camera->m_FarPlane / camera->m_NearPlane));
	
		m_CBOs[frameIndex].xFov = camera->m_Fov;
		m_CBOs[frameIndex].yFov = camera->m_Fov * ((float)extent.height / (float)extent.width);
	}

	VkDescriptorSetLayout CameraBuffer::GetLayout()
	{
		return Context::Get().m_DescriptorAllocator->MakeLayout(DescriptorInfo{
			std::vector<DescriptorInfo::ImageInfo>{},
			std::vector<DescriptorInfo::BufferInfo>{{
				.index = 0U,
				.type  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.stage = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT,
			}}
		});
	}
	Handle<DescriptorSet> CameraBuffer::GetDescriptorHandle(uint32_t frameIndex)
	{
		return m_DescriptorSets[frameIndex];
	}
}