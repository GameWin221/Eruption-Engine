#pragma once

#include <Common/Helpers.hpp>

#ifndef EN_CAMERAMATRICESBUFFER_HPP
#define EN_CAMERAMATRICESBUFFER_HPP

namespace en
{
	class CameraMatricesBuffer
	{
	public:
		CameraMatricesBuffer();
		~CameraMatricesBuffer();

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);

		static VkDescriptorSetLayout& GetLayout();

		struct CameraMatricesBufferObject
		{
			alignas(16) glm::mat4 view = glm::mat4(1.0f);
			alignas(16) glm::mat4 proj = glm::mat4(1.0f);
		} m_Matrices;

	private:
		void CreateDescriptorSet();

		VkDescriptorSet m_DescriptorSet;

		VkBuffer       m_Buffer;
		VkDeviceMemory m_BufferMemory;
		VkDeviceSize   m_BufferSize;
	};
}
#endif