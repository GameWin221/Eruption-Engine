#pragma once

#ifndef EN_RENDERER_HPP
#define EN_RENDERER_HPP

#include "../../EruptionEngine.ini"

#include <Common/Helpers.hpp>

#include <Scene/Scene.hpp>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>
#include <Renderer/Passes/Pass.hpp>
#include <Renderer/Passes/GraphicsPass.hpp>
#include <Renderer/Passes/ComputePass.hpp>

#include <Renderer/Sampler.hpp>

#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

#include <Renderer/Camera/Camera.hpp>
#include <Renderer/Camera/CameraBuffer.hpp>

#include <Renderer/DescriptorSet.hpp>

#include <Renderer/ImGuiContext.hpp>

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

		enum struct QualityLevel
		{
			Low = 0,
			Medium = 1,
			High = 2,
			Ultra = 3
		};

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

		enum struct AmbientOcclusionMode
		{
			None = 0,
			SSAO = 1
		};

		struct AmbientOcclusionProperties
		{
			float screenWidth = 1920.0f;
			float screenHeight = 1080.0f;
			
			float radius = 0.85f;
			float bias = 0.025f;
			float multiplier = 1.2f;

			uint32_t _samples = 64U;
			float _noiseScale = 4.0f;
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

		void SetAmbientOcclusionMode(const AmbientOcclusionMode aoMode);
		const AmbientOcclusionMode GetAmbientOcclusionMode() const { return m_Settings.ambientOcclusionMode; };

		AntialiasingProperties& GetAntialiasingProperties() { return m_Settings.antialiasing; };
		AmbientOcclusionProperties& GetAmbientOcclusionProperties() { return m_Settings.ambientOcclusion; };

		//void SetAntialaliasingQuality(const QualityLevel quality);
		//const QualityLevel GetAntialaliasingQuality() const { return m_Settings.antialiasingQuality; };

		void SetAmbientOcclusionQuality(const QualityLevel quality);
		const QualityLevel GetAmbientOcclusionQuality() const { return m_Settings.ambientOcclusionQuality; };

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

		Handle<GraphicsPass> m_SSAOPass;
		Handle<GraphicsPass> m_ForwardPass;
		Handle<GraphicsPass> m_AntialiasingPass;
		Handle<GraphicsPass> m_DepthPass;

		Handle<GraphicsPass> m_PointShadowPass;
		Handle<GraphicsPass> m_SpotShadowPass;
		Handle<GraphicsPass> m_DirShadowPass;

		Handle<ComputePass> m_ClusterAABBCreationPass;
		Handle<ComputePass> m_ClusterLightCullingPass;

		Handle<Sampler> m_ShadowSampler;
		Handle<Sampler> m_FullscreenSampler;

		Handle<DescriptorSet> m_ShadowMapsDescriptor;
		Handle<DescriptorSet> m_ClusterDescriptor;
		Handle<DescriptorSet> m_AntialiasingDescriptor;
		Handle<DescriptorSet> m_SSAODescriptor;
		Handle<DescriptorSet> m_DepthBufferDescriptor;

		std::array<Handle<Image>, MAX_POINT_LIGHT_SHADOWS> m_PointShadowMaps;
		std::array<Handle<Image>, MAX_SPOT_LIGHT_SHADOWS> m_SpotShadowMaps;
		std::array<Handle<Image>, MAX_DIR_LIGHT_SHADOWS * SHADOW_CASCADES> m_DirShadowMaps;

		Handle<Image> m_DepthBuffer;
		Handle<Image> m_SSAOTarget;
		Handle<Image> m_PointShadowDepthBuffer;
		Handle<Image> m_SpotShadowDepthBuffer;
		Handle<Image> m_AliasedImage;

		Handle<ImGuiContext> m_ImGuiContext;

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
			AntialiasingProperties antialiasing{};

			AmbientOcclusionMode ambientOcclusionMode = AmbientOcclusionMode::SSAO;
			AmbientOcclusionProperties ambientOcclusion{};
			QualityLevel ambientOcclusionQuality = QualityLevel::High;
		} m_Settings;

		struct CSM {
			std::array<std::array<glm::mat4, SHADOW_CASCADES>, MAX_DIR_LIGHT_SHADOWS> cascadeMatrices{};

			std::array<float, SHADOW_CASCADES> cascadeSplitDistances{};
			std::array<float, SHADOW_CASCADES> cascadeFrustumSizeRatios{};

			std::array<glm::vec3, SHADOW_CASCADES> cascadeCenters{};
			std::array<float, SHADOW_CASCADES> cascadeRadiuses{};
		} m_CSM;

		struct ClusterSSBOs {
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
		void SSAOPass();
		void ForwardPass();
		void AntialiasingPass();
		void ImGuiPass();
		void EndRender();

		void UpdateCSM();

		void CreateShadowResources();

		void CreateShadowPasses();
		void CreateSSAOPass();
		void CreateDepthPass();
		void CreateForwardPass();
		void CreateAntialiasingPass();

		void CreateClusterBuffers();
		void CreateClusterPasses();

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);
		void RecreateFramebuffer();
		void ReloadBackendImpl();

		void CreateBackend(bool newImGui = true);

		void CreateAATarget();
		void CreateSSAOTarget();
		void CreateDepthBuffer();

		void CreatePerFrameData();
		void DestroyPerFrameData();
	};
}

#endif