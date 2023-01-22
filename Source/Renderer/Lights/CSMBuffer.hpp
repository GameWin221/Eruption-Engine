#pragma once

#ifndef EN_CSMBUFFER_HPP
#define EN_CSMBUFFER_HPP

#include "../EruptionEngine.ini"
#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Renderer/Camera/Camera.hpp>
#include <Renderer/DescriptorSet.hpp>

namespace en
{
	class CSMBuffer
	{
	public:
		CSMBuffer();

		void MapBuffer(uint32_t frameIndex);

		VkDescriptorSetLayout GetLayout();

		Handle<DescriptorSet> GetDescriptorHandle(uint32_t frameIndex);

		struct CSMBufferObject
		{
			glm::vec4 cascadeSplitDistances[SHADOW_CASCADES]{};
			glm::vec4 cascadeFrustumSizeRatios[SHADOW_CASCADES]{};

			glm::mat4 cascadeLightMatrices[MAX_DIR_LIGHTS][SHADOW_CASCADES]{};
		};

		std::array<CSMBufferObject, FRAMES_IN_FLIGHT> m_CSMBOs;

	private:
		std::array<Handle<DescriptorSet>, FRAMES_IN_FLIGHT> m_DescriptorSets;
		std::array<Handle<MemoryBuffer>, FRAMES_IN_FLIGHT> m_Buffers;
	};
}

#endif