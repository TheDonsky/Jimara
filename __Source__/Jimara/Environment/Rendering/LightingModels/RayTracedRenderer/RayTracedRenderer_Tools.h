#pragma once
#include "RayTracedRenderer.h"
#include "../Utilities/GraphicsObjectPipelines.h"
#include "../Utilities/IndexedGraphicsObjectDataProvider.h"
#include "../../TransientImage.h"
#include "../../SceneObjects/Lights/SceneLightGrid.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"


namespace Jimara {
	struct RayTracedRenderer::Tools {
		struct FrameBuffers;
		class FrameBufferManager;
		struct ViewportBuffer;
		class SharedBindings;
		class RasterPass;
		class RayTracedPass;
		class Renderer;

		static const constexpr bool USE_HARDWARE_MULTISAMPLING = false;
		static const constexpr Graphics::Texture::PixelFormat PRIMITIVE_RECORD_ID_FORMAT = Graphics::Texture::PixelFormat::R32G32B32A32_UINT;
		static const constexpr std::string_view LIGHTING_MODEL_PATH = "Jimara/Environment/Rendering/LightingModels/RayTracedRenderer/Jimara_RayTracedRenderer.jlm";

		static const constexpr std::string_view LIGHT_DATA_BUFFER_NAME = "jimara_LightDataBinding";

		static const constexpr std::string_view VIEWPORT_BUFFER_NAME = "jimara_RayTracedRenderer_ViewportBuffer";
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



	struct RayTracedRenderer::Tools::ViewportBuffer {
		alignas(16) Matrix4 view = Math::Identity();
		alignas(16) Matrix4 projection = Math::Identity();
		alignas(16) Matrix4 viewPose = Math::Identity();
	};

	class RayTracedRenderer::Tools::SharedBindings : public virtual Object {
	public:
		// Bindless:
		const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> bindlessBuffers;
		const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> bindlessSamplers;

		// Light grid:
		const Reference<SceneLightGrid> lightGrid;
		const Reference<LightDataBuffer> lightDataBuffer;
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> lightDataBinding = 
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
		const Reference<LightTypeIdBuffer> lightTypeIdBuffer;
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> lightTypeIdBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

		// Viewport:
		const Reference<const ViewportDescriptor> viewport;
		const Reference<Graphics::BindingPool> bindingPool;
		const Graphics::BufferReference<ViewportBuffer> viewportBuffer;
		const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> viewportBinding;
		ViewportBuffer viewportBufferData = {};
		Vector3 eyePosition = Vector3(0.0f);


		static Reference<SharedBindings> Create(const ViewportDescriptor* viewport);

		void Update();

		Reference<Graphics::BindingSet> CreateBindingSet(Graphics::Pipeline* pipeline, size_t bindingSetId)const;

	private:
		SharedBindings(
			SceneLightGrid* sceneLightGrid, 
			LightDataBuffer* sceneLightDataBuffer,
			LightTypeIdBuffer* sceneLightTypeIdBuffer,
			const ViewportDescriptor* viewportDesc, 
			Graphics::BindingPool* pool,
			const Graphics::ResourceBinding<Graphics::Buffer>* viewportData);
	};



	class RayTracedRenderer::Tools::RasterPass : public virtual Object {
	public:
		virtual ~RasterPass();

		static Reference<RasterPass> Create(
			const RayTracedRenderer* renderer,
			const SharedBindings* sharedBindings,
			LayerMask layers, Graphics::RenderPass::Flags flags);

		bool SetFrameBuffers(const FrameBuffers& frameBuffers);

		bool Render(Graphics::InFlightBufferInfo commandBufferInfo);

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjects;
		const Reference<IndexedGraphicsObjectDataProvider> m_objectDescProvider;
		const Reference<const SharedBindings> m_sharedBindings;
		const LayerMask m_layers;
		const Graphics::RenderPass::Flags m_flags;
		Reference<Graphics::RenderPass> m_renderPass;
		Reference<GraphicsObjectPipelines> m_pipelines;
		Stacktor<Reference<Graphics::BindingSet>, 4u> m_environmentBindings;
		Reference<Graphics::TextureView> m_primitiveRecordBuffer;
		Reference<Graphics::TextureView> m_depthBuffer;
		Reference<Graphics::FrameBuffer> m_frameBuffer;

		// Constructor can only be invoked internally..
		RasterPass(
			GraphicsObjectDescriptor::Set* graphicsObjects,
			IndexedGraphicsObjectDataProvider* objectDescProvider, 
			const SharedBindings* sharedBindings,
			const LayerMask& layers, Graphics::RenderPass::Flags flags);

		// Private stuff resides in-here
		struct Helpers;
	};

	class RayTracedRenderer::Tools::RayTracedPass : public virtual Object {
	public:
		virtual ~RayTracedPass();

		static Reference<RayTracedPass> Create(
			const RayTracedRenderer* renderer,
			const SharedBindings* sharedBindings,
			LayerMask layers);

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
