#pragma once

#ifndef EN_CAMERAMATRICESBUFFER_HPP
#define EN_CAMERAMATRICESBUFFER_HPP

#include "../../EruptionEngine.ini"

#include <Renderer/Buffers/MemoryBuffer.hpp>

#include "Camera.hpp"

namespace en
{
	class CameraMatricesBuffer
	{
	public:
		CameraMatricesBuffer();
		~CameraMatricesBuffer();

		void MapBuffer(uint32_t frameIndex);

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, uint32_t& frameIndex);

		static VkDescriptorSetLayout& GetLayout();

		struct CameraMatricesBufferObject
		{
			glm::mat4 view = glm::mat4(1.0f);
			glm::mat4 proj = glm::mat4(1.0f);
		};

		std::array<CameraMatricesBufferObject, FRAMES_IN_FLIGHT> m_Matrices;

	private:
		void CreateDescriptorSets();

		std::array<VkDescriptorSet, FRAMES_IN_FLIGHT> m_DescriptorSets;

		std::array<std::unique_ptr<MemoryBuffer>, FRAMES_IN_FLIGHT> m_Buffers;
	};
}
#endif