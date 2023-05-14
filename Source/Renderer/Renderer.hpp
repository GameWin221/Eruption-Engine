#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "../../EruptionEngine.ini"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <Common/Helpers.hpp>

#include <Scene/Scene.hpp>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Pipelines/Pipeline.hpp>
#include <Renderer/Pipelines/GraphicsPipeline.hpp>
#include <Renderer/Pipelines/ComputePipeline.hpp>

#include <Renderer/RenderPass.hpp>
#include <Renderer/Framebuffer.hpp>
#include <Renderer/Sampler.hpp>

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

#include <Renderer/Camera/Camera.hpp>
#include <Renderer/Camera/CameraBuffer.hpp>

#include <Renderer/DescriptorSet.hpp>

#include <functional>
#include <random>

#include <Renderer/Swapchain.hpp>

namespace en
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void BindScene(en::Handle<Scene> scene);
		void UnbindScene();

		enum struct AntialiasingMode
		{
			None = 0,
			FXAA = 1
		};

		struct AntialiasingProperties
		{
			float fxaaSpanMax = 4.0f;
			float fxaaReduceMin = 0.001f;
			float fxaaReduceMult = 0.050f;
			float fxaaPower = 1.0f;

			float texelSizeX = 1.0f / 1920.0f;
			float texelSizeY = 1.0f / 1080.0f;
		};

		void SetVSyncEnabled(const bool enabled);
		const bool GetVSyncEnabled() const { return m_Settings.vSync; };

		void SetDepthPrepassEnabled(const bool enabled);
		const bool GetDepthPrepassEnabled() const { return m_Settings.depthPrePass; };

		void SetShadowCascadesWeight(const float weight);
		const float GetShadowCascadesWeight() const { return m_Settings.cascadeSplitWeight; };

		void SetShadowCascadesFarPlane(const float farPlane);
		const float GetShadowCascadesFarPlane() const { return m_Settings.cascadeFarPlane; };

		void SetPointShadowResolution(const uint32_t resolution);
		const float GetPointShadowResolution() const { return m_Settings.pointLightShadowResolution; };

		void SetSpotShadowResolution(const uint32_t resolution);
		const float GetSpotShadowResolution() const { return m_Settings.spotLightShadowResolution; };

		void SetDirShadowResolution(const uint32_t resolution);
		const float GetDirShadowResolution() const { return m_Settings.dirLightShadowResolution; };

		void SetShadowFormat(const VkFormat format);
		const VkFormat GetShadowFormat() const { return m_Settings.shadowsFormat; };

		void SetAntialiasingMode(const AntialiasingMode antialiasingMode);
		const AntialiasingMode GetAntialiasingMode() const { return m_Settings.antialiasingMode; };

		AntialiasingProperties& GetAntialiasingProperties() { return m_Settings.antialiasing; };

		void ReloadBackend();

		void Update();
		void PreRender();
		void Render();

		en::Handle<Scene> GetScene() { return m_Scene; };

		const double GetFrameTime() const { return m_FrameTime; }

		int m_DebugMode = 0;

		std::function<void()> m_ImGuiRenderCallback;

	private:
		Handle<Scene> m_Scene;

		Handle<Swapchain> m_Swapchain;
		Handle<CameraBuffer> m_CameraBuffer;

		Handle<RenderPass> m_ImGuiRenderPass;
		Handle<RenderPass> m_ForwardPass;
		Handle<RenderPass> m_AntialiasingPass;
		Handle<RenderPass> m_DepthRenderPass;

		Handle<RenderPass> m_DirShadowsRenderPass;
		Handle<RenderPass> m_PerspectiveShadowsRenderPass;

		Handle<Framebuffer> m_DepthFramebuffer;

		Handle<Sampler> m_ShadowSampler;
		Handle<Sampler> m_FullscreenSampler;

		Handle<DescriptorSet> m_ShadowMapsDescriptor;
		Handle<DescriptorSet> m_ClusterDescriptor;
		Handle<DescriptorSet> m_AntialiasingDescriptor;

		std::vector<Handle<Framebuffer>> m_ImGuiFramebuffers;
		std::vector<Handle<Framebuffer>> m_ForwardFramebuffers;
		std::vector<Handle<Framebuffer>> m_AntialiasingFramebuffers;

		std::array<OmniShadowMap, MAX_POINT_LIGHT_SHADOWS> m_PointShadowMaps;
		std::array<ShadowMap, MAX_SPOT_LIGHT_SHADOWS> m_SpotShadowMaps;
		std::array<ShadowMap, MAX_DIR_LIGHT_SHADOWS* SHADOW_CASCADES> m_DirShadowMaps;

		Handle<GraphicsPipeline> m_Pipeline;
		Handle<GraphicsPipeline> m_AntialiasingPipeline;
		Handle<GraphicsPipeline> m_DepthPipeline;

		Handle<GraphicsPipeline> m_DirShadowPipeline;
		Handle<GraphicsPipeline> m_SpotShadowPipeline;
		Handle<GraphicsPipeline> m_PointShadowPipeline;

		Handle<ComputePipeline> m_ClusterAABBCreation;
		Handle<ComputePipeline> m_ClusterLightCulling;

		Handle<Image> m_DepthBuffer;
		Handle<Image> m_PointShadowDepthBuffer;
		Handle<Image> m_SpotShadowDepthBuffer;
		Handle<Image> m_AliasedImage;

		struct Settings {
			bool vSync = true;

			bool depthPrePass = true;

			float cascadeSplitWeight = 0.87f;
			float cascadeFarPlane = 140.0f;

			uint32_t pointLightShadowResolution = 512U;
			uint32_t spotLightShadowResolution = 1024U;
			uint32_t dirLightShadowResolution = 2048U;

			VkFormat shadowsFormat = VK_FORMAT_D32_SFLOAT;

			AntialiasingMode antialiasingMode = AntialiasingMode::FXAA;
			AntialiasingProperties antialiasing;
		} m_Settings;

		struct CSM {
			std::array<std::array<glm::mat4, SHADOW_CASCADES>, MAX_DIR_LIGHT_SHADOWS> cascadeMatrices{};

			std::array<float, SHADOW_CASCADES> cascadeSplitDistances{};
			std::array<float, SHADOW_CASCADES> cascadeFrustumSizeRatios{};

			std::array<glm::vec3, SHADOW_CASCADES> cascadeCenters{};
			std::array<float, SHADOW_CASCADES> cascadeRadiuses{};
		} m_CSM;

		struct ClusterSSBOs
		{
			Handle<MemoryBuffer> aabbClusters;
			Handle<DescriptorSet> aabbClustersDescriptor;

			Handle<MemoryBuffer> pointLightGrid;
			Handle<MemoryBuffer> pointLightIndices;
			Handle<MemoryBuffer> pointLightGlobalIndexOffset;

			Handle<MemoryBuffer> spotLightGrid;
			Handle<MemoryBuffer> spotLightIndices;
			Handle<MemoryBuffer> spotLightGlobalIndexOffset;

			Handle<DescriptorSet> clusterLightCullingDescriptor;
		} m_ClusterSSBOs;

		struct Frame {
			VkCommandBuffer commandBuffer;
			VkFence submitFence;

			VkSemaphore mainSemaphore;
			VkSemaphore presentSemaphore;
		} m_Frames[FRAMES_IN_FLIGHT];
	
		uint32_t m_FrameIndex = 0U;
			
		bool m_ReloadQueued		  = false;
		bool m_FramebufferResized = false;
		bool m_SkipFrame		  = false;

		bool m_ClusterFrustumChanged = false;

		double m_FrameTime{};

		void WaitForActiveFrame();
		void ResetAllFrames();

		void MeasureFrameTime();
		void BeginRender();
		void ShadowPass();
		void ClusterComputePass();
		void DepthPass();
		void ForwardPass();
		void AntialiasingPass();
		void ImGuiPass();
		void EndRender();

		void UpdateCSM();

		void CreateShadowResources();

		void CreateShadowPasses();
		void CreateShadowPipelines();

		void CreateDepthPass();
		void CreateDepthPipeline();

		void CreateForwardPass();
		void CreateForwardPipeline();

		void CreateAntialiasingPass();
		void CreateAntialiasingPipeline();

		void CreateClusterBuffers();
		void CreateClusterPipelines();

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();
		void ReloadBackendImpl();

		void CreateBackend(bool newImGui = true);

		void CreateDepthBuffer();

		void CreatePerFrameData();
		void DestroyPerFrameData();

		void CreateImGuiRenderPass();
		void CreateImGuiContext();
		void DestroyImGuiContext();
	};
}

#endif