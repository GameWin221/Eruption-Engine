#pragma once
#ifndef EN_CAMERABUFFER_HPP
#define EN_CAMERABUFFER_HPP

#include "../EruptionEngine.ini"
#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Renderer/Camera/Camera.hpp>
#include <Renderer/DescriptorSet.hpp>

namespace en
{
	class CameraBuffer
	{
	public:
		CameraBuffer();
		~CameraBuffer(){};

		void MapBuffer(uint32_t frameIndex);

		void UpdateBuffer(uint32_t frameIndex, Handle<Camera> camera, VkExtent2D extent, int debugMode);

		VkDescriptorSetLayout GetLayout();

		Handle<DescriptorSet> GetDescriptorHandle(uint32_t frameIndex);

		struct CameraBufferObject
		{
			glm::mat4 view = glm::mat4(1.0f);
			glm::mat4 invView = glm::mat4(1.0f);
			glm::mat4 proj = glm::mat4(1.0f);
			glm::mat4 invProj = glm::mat4(1.0f);
			glm::mat4 invProjView = glm::mat4(1.0f);
			glm::mat4 projView = glm::mat4(1.0f);

			glm::uvec4 clusterTileCount = glm::uvec4(0U);
			glm::uvec4 clusterTileSizes = glm::uvec4(0U);

			float clusterScale = 0.0f;
			float clusterBias = 0.0f;

			float zNear = 0.0f;
			float zFar = 1.0f;

			glm::vec3 position = glm::vec3(0.0f);

			int debugMode = 0;
		};

		std::array<CameraBufferObject, FRAMES_IN_FLIGHT> m_CBOs;

	private:
		std::array<Handle<DescriptorSet>, FRAMES_IN_FLIGHT> m_DescriptorSets;
		std::array<Handle<MemoryBuffer>, FRAMES_IN_FLIGHT> m_Buffers;
	};
}
#endif