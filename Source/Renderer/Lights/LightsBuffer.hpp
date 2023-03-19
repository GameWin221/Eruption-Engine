#pragma once
#ifndef EN_LIGHTSBUFFER_HPP
#define EN_LIGHTSBUFFER_HPP

#include "EruptionEngine.ini"
#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Renderer/DescriptorSet.hpp>

#include <Renderer/Camera/Camera.hpp>
#include <Scene/Scene.hpp>

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

namespace en
{
	class LightsBuffer
	{
	public:
		LightsBuffer();

		void MapStagingMemory(uint32_t frameIndex);

		void CopyStagingToDevice(uint32_t frameIndex, VkCommandBuffer cmd);

		struct LightsBufferObject
		{
			PointLight::Buffer		 pointLights[MAX_POINT_LIGHTS]{};
			SpotLight::Buffer		 spotLights[MAX_SPOT_LIGHTS]{};
			DirectionalLight::Buffer dirLights[MAX_DIR_LIGHTS]{};

			uint32_t activePointLights = 0U;
			uint32_t activeSpotLights = 0U;
			uint32_t activeDirLights = 0U;
			uint32_t _dummy0;

			glm::vec3 ambientLight = glm::vec3(0.0f);
			uint32_t _dummy1;

		} LBO;

		std::array<LightsBufferObject, FRAMES_IN_FLIGHT> m_LBOs;

		inline const VkBuffer     GetHandle() const { return m_Buffer->GetHandle();  }
		inline const VkDeviceSize GetSize()   const { return m_Buffer->GetSize(); }

	private:
		std::array<Handle<MemoryBuffer>, FRAMES_IN_FLIGHT> m_StagingBuffers;
		Handle<MemoryBuffer> m_Buffer;
	};
}


#endif