#pragma once
#include "RayTracedRenderer.h"
#include "../Utilities/GraphicsObjectPipelines.h"
#include "../Utilities/IndexedGraphicsObjectDataProvider.h"
#include "../Utilities/JM_StandardVertexInputStructure.h"
#include "../Utilities/GraphicsObjectAccelerationStructure.h"
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

		struct PerObjectData;
		enum class ObjectDataFlags : uint32_t;
		template<typename VertexInputType>
		struct PerObjectResources;
		class SceneObjectData;

		class RasterPass;
		class RayTracedPass;
		class Renderer;

		static const constexpr bool USE_HARDWARE_MULTISAMPLING = false;
		static const constexpr Graphics::Texture::PixelFormat PRIMITIVE_RECORD_ID_FORMAT = Graphics::Texture::PixelFormat::R32G32B32A32_UINT;
		
		static const constexpr uint32_t JM_RT_FLAG_MATERIAL_NOT_IN_RT_PIPELINE = ~uint32_t(0u);
		static const constexpr uint32_t JM_RT_FLAG_EDGES = (1u << 0u);

		static const constexpr std::string_view LIGHTING_MODEL_PATH = "Jimara/Environment/Rendering/LightingModels/RayTracedRenderer/Jimara_RayTracedRenderer.jlm";

		static const constexpr std::string_view RASTER_PASS_STAGE_NAME = "RasterPass";
		static const constexpr std::string_view RAY_GEN_STAGE_NAME = "RayGeneration";
		static const constexpr std::string_view SHADE_FRAGMENT_CALL_NAME = "ShadeFragment_Call";
		static const constexpr std::string_view TRANSPARENCY_QUERY_CALL_NAME = "TransparencyQuery_Call";

		static const constexpr std::string_view LIGHT_DATA_BUFFER_NAME = "jimara_LightDataBinding";
		static const constexpr std::string_view LIGHT_TYPE_IDS_BUFFER_NAME = "jimara_RayTracedRenderer_LightTypeIds";
		static const constexpr std::string_view VIEWPORT_BUFFER_NAME = "jimara_RayTracedRenderer_ViewportBuffer";
		static const constexpr std::string_view SCENE_OBJECT_DATA_BUFFER_NAME = "jimara_RayTracedRenderer_SceneObjectData";

		static const constexpr std::string_view PRIMITIVE_RECORD_ID_BINDING_NAME = "JM_RayTracedRenderer_primitiveRecordId";
		static const constexpr std::string_view FRAME_COLOR_BINDING_NAME = "JM_RayTracedRenderer_frameColor";
		static const constexpr std::string_view TLAS_BINDING_BAME = "JM_RayTracedRenderer_tlas";
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
		alignas(16) Matrix4 inverseProjection = Math::Identity();
		alignas(4u) uint32_t rasterizedGeometrySize = 0u;
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

		// Per-Object-Data binding (updated externally):
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> perObjectDataBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();


		static Reference<SharedBindings> Create(const ViewportDescriptor* viewport);

		void Update(uint32_t rasterizedGeometrySize);

		Reference<Graphics::BindingSet> CreateBindingSet(
			Graphics::Pipeline* pipeline, size_t bindingSetId, 
			const Graphics::BindingSet::BindingSearchFunctions& additionalSearchFunctions = {})const;

	private:
		SharedBindings(
			SceneLightGrid* sceneLightGrid, 
			LightDataBuffer* sceneLightDataBuffer,
			LightTypeIdBuffer* sceneLightTypeIdBuffer,
			const ViewportDescriptor* viewportDesc, 
			Graphics::BindingPool* pool,
			const Graphics::ResourceBinding<Graphics::Buffer>* viewportData);
	};





	struct RayTracedRenderer::Tools::PerObjectData {
		alignas(8) JM_StandardVertexInput vertexInput;                            // Vertex input data;                       Bytes [0 - 112)
		alignas(8) uint64_t indexBufferId = 0u;                                   // Index buffer device address;             Bytes [112 - 120)

		alignas(8) uint64_t materialSettingsBufferId = 0u;                        // Material settings buffer device-address; Bytes [120 - 128)
		alignas(4) uint32_t materialId = JM_RT_FLAG_MATERIAL_NOT_IN_RT_PIPELINE;  // Material index;                          Bytes [128 - 132)

		alignas(4) uint32_t flags = 0u;                                           // Additional flags;                        Bytes [132 - 136)

		alignas(4) uint32_t firstBlasInstance = 0u;                               // First BLAS instance id inside TLAS       Bytes [136 - 140)
		alignas(4) uint32_t pad_1 = 0u;                                           // Padding;                                 Bytes [140 - 144)
	};

	template<typename VertexInputType>
	struct RayTracedRenderer::Tools::PerObjectResources {
		Reference<const GraphicsObjectDescriptor::ViewportData> viewportData;
		
		uint32_t materialId = JM_RT_FLAG_MATERIAL_NOT_IN_RT_PIPELINE;
		Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> materialSettingsBuffer;
		uint32_t flags = 0u;
		
		VertexInputType vertexInput;

		// ObjectId in case of the raster objects for indirection or firstBlasInstance for blas instance.
		uint32_t indirectObjectIndex = 0u;
	};





	class RayTracedRenderer::Tools::RasterPass : public virtual Object {
	public:
		virtual ~RasterPass();

		static Reference<RasterPass> Create(
			const RayTracedRenderer* renderer,
			const SharedBindings* sharedBindings,
			LayerMask layers, Graphics::RenderPass::Flags flags);

		bool SetFrameBuffers(const FrameBuffers& frameBuffers);

		class State final {
		public:
			State(const RasterPass* pass);

			~State();

			inline const GraphicsObjectPipelines::Reader& Pipelines()const { return m_pipelines; };

			bool Render(Graphics::InFlightBufferInfo commandBufferInfo);

		private:
			Reference<const RasterPass> m_pass;
			const GraphicsObjectPipelines::Reader m_pipelines;
		};

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

		class State final {
		public:
			State(const RayTracedPass* pass);

			~State();

			inline Graphics::RayTracingPipeline* Pipeline()const { return m_pass->m_pipeline; }

			inline const GraphicsObjectAccelerationStructure::Reader& Tlas()const { return m_tlas; };

			bool Render(Graphics::InFlightBufferInfo commandBufferInfo);

			uint32_t MaterialIndex(const Material::LitShader* litShader)const;

		private:
			Reference<const RayTracedPass> m_pass;
			const GraphicsObjectAccelerationStructure::Reader m_tlas;
		};

		void GetDependencies(Callback<JobSystem::Job*> report);

	private:
		const Reference<const SharedBindings> m_sharedBindings;
		const Reference<GraphicsObjectAccelerationStructure> m_accelerationStructure;
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_primitiveRecordIdBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> m_frameColorBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
		const Reference<Graphics::ResourceBinding<Graphics::TopLevelAccelerationStructure>> m_tlasBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TopLevelAccelerationStructure>>();

		std::unordered_map<Reference<const Material::LitShader>, uint32_t> m_materialIndex;
		std::vector<Reference<const Material::LitShader>> m_materialByIndex;

		Reference<Graphics::RayTracingPipeline> m_pipeline;
		Stacktor<Reference<Graphics::BindingSet>, 4u> m_pipelineBindings;

		// Constructor can only be invoked internally..
		RayTracedPass(const SharedBindings* sharedBindings, GraphicsObjectAccelerationStructure* accelerationStructure);

		// Private stuff resides in-here
		struct Helpers;
	};


	class RayTracedRenderer::Tools::SceneObjectData : public virtual Object {
	public:
		SceneObjectData(SharedBindings* sharedBindings);

		virtual ~SceneObjectData();

		bool Update(
			const GraphicsObjectPipelines::Reader& rasterPipelines,
			const RayTracedPass::State& rtPass,
			std::vector<Reference<const Object>>& resourceList);

		const uint32_t RasterizedGeometrySize()const { return m_rasterizedGeometrySize; }

	private:
		const Reference<SceneContext> m_context;
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_perObjectDataBinding;
		uint32_t m_rasterizedGeometrySize = 0u;

		Reference<Graphics::RayTracingPipeline> m_lastRTPipeline;
		std::vector<PerObjectResources<JM_StandardVertexInput::Extractor>> m_rasterizedGeometryResources;
		std::vector<PerObjectResources<GraphicsObjectDescriptor::GeometryDescriptor>> m_tlasGeometryResources;

		struct Helpers;
	};
}
