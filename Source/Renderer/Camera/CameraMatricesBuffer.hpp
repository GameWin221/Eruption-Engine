#pragma once

#ifndef EN_CAMERAMATRICESBUFFER_HPP
#define EN_CAMERAMATRICESBUFFER_HPP

#include <Renderer/Buffers/MemoryBuffer.hpp>

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
			glm::mat4 view = glm::mat4(1.0f);
			glm::mat4 proj = glm::mat4(1.0f);
		} m_Matrices;

	private:
		void CreateDescriptorSet();

		VkDescriptorSet m_DescriptorSet;

		std::unique_ptr<MemoryBuffer> m_Buffer;
	};
}
#endif