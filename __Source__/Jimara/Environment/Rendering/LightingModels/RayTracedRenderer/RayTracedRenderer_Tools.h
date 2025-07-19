#pragma once
#include "RayTracedRenderer.h"
#include "../Utilities/GraphicsObjectPipelines.h"
#include "../Utilities/IndexedGraphicsObjectDataProvider.h"
#include "../../TransientImage.h"


namespace Jimara {
	struct RayTracedRenderer::Tools {
		struct FrameBuffers;
		class FrameBufferManager;
		class RasterPass;
		class RayTracedPass;
		class Renderer;

		static const constexpr bool USE_HARDWARE_MULTISAMPLING = false;
		static const constexpr Graphics::Texture::PixelFormat PRIMITIVE_RECORD_ID_FORMAT = Graphics::Texture::PixelFormat::R32G32B32A32_UINT;
		static const constexpr std::string_view LIGHTING_MODEL_PATH = "Jimara/Environment/Rendering/LightingModels/ObjectIdRenderer/Jimara_RayTracedRenderer.jlm";
	};

	struct RayTracedRenderer::Tools::FrameBuffers {
		Reference<Graphics::TextureView> primitiveRecordId;
		Reference<Graphics::TextureView> colorTexture;
		Reference<Graphics::TextureView> depthBuffer;
	};


	class RayTracedRenderer::Tools::FrameBufferManager : public virtual Object {
	public:
		class Lock;

		FrameBufferManager(SceneContext* context);

		virtual ~FrameBufferManager();

	private:
		const Reference<SceneContext> m_context;
		std::mutex m_lock;
		Reference<RenderImages> m_lastRenderImages;
		Reference<TransientImage> m_lastPrimitiveRecordId;
		FrameBuffers m_buffers;
	};

	class RayTracedRenderer::Tools::FrameBufferManager::Lock final {
	public:
		Lock(FrameBufferManager* manager, RenderImages* images);

		~Lock();

		bool Good()const;

		inline FrameBuffers Buffers()const { return m_manager->m_buffers; }

	private:
		const Reference<FrameBufferManager> m_manager;
		std::unique_lock<decltype(FrameBufferManager::m_lock)> m_lock;

		struct Helpers;
	};

	class RayTracedRenderer::Tools::RasterPass : public virtual Object {
	public:
		virtual ~RasterPass();

		static Reference<RasterPass> Create(
			const RayTracedRenderer* renderer, 
			const ViewportDescriptor* viewport, 
			LayerMask layers, Graphics::RenderPass::Flags flags);

		bool SetFrameBuffers(const FrameBuffers& frameBuffers);

		bool Render(Graphics::InFlightBufferInfo commandBufferInfo);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		const Reference<const ViewportDescriptor> m_viewport;
		const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjects;
		const Reference<IndexedGraphicsObjectDataProvider> m_objectDescProvider;
		const LayerMask m_layers;
		const Graphics::RenderPass::Flags m_flags;
		Reference<Graphics::RenderPass> m_renderPass;
		Reference<GraphicsObjectPipelines> m_pipelines;
		Reference<Graphics::TextureView> m_primitiveRecordBuffer;
		Reference<Graphics::TextureView> m_depthBuffer;
		Reference<Graphics::FrameBuffer> m_frameBuffer;

		// Constructor can only be invoked internally..
		RasterPass(const ViewportDescriptor* viewport, 
			GraphicsObjectDescriptor::Set* graphicsObjects, 
			IndexedGraphicsObjectDataProvider* objectDescProvider, 
			const LayerMask& layers, Graphics::RenderPass::Flags flags);

		// Private stuff resides in-here
		struct Helpers;
	};

	class RayTracedRenderer::Tools::RayTracedPass : public virtual Object {
	public:
		virtual ~RayTracedPass();

		static Reference<RayTracedPass> Create(const RayTracedRenderer* renderer, const ViewportDescriptor* viewport, LayerMask layers);

		bool SetFrameBuffers(const FrameBuffers& frameBuffers);

		bool Render(Graphics::InFlightBufferInfo commandBufferInfo);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		// Constructor can only be invoked internally..
		RayTracedPass();

		// Private stuff resides in-here
		struct Helpers;
	};
}
