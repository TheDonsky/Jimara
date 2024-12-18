#include "ObjectIdRenderer.h"
#include "../Utilities/IndexedGraphicsObjectDataProvider.h"


namespace Jimara {
	namespace {
		struct ObjectIdRenderer_Configuration {
			Reference<const ViewportDescriptor> descriptor;
			LayerMask layerMask;

			inline bool operator==(const ObjectIdRenderer_Configuration& other)const {
				return descriptor == other.descriptor && layerMask == other.layerMask;
			}

			inline bool operator<(const ObjectIdRenderer_Configuration& other)const {
				return descriptor < other.descriptor || (descriptor == other.descriptor && layerMask < other.layerMask);
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::ObjectIdRenderer_Configuration> {
		size_t operator()(const Jimara::ObjectIdRenderer_Configuration& desc)const {
			return Jimara::MergeHashes(
				std::hash<const Jimara::ViewportDescriptor*>()(desc.descriptor),
				std::hash<Jimara::LayerMask>()(desc.layerMask));
		}
	};
}


namespace Jimara {
	struct ObjectIdRenderer::Helpers {
		/** Render pass constants: */
		inline static float UintAsFloatBytes(uint32_t value) { return *reinterpret_cast<float*>(&value); }
		static const constexpr size_t VERTEX_POSITION_ATTACHMENT_ID = 0u;
		static const constexpr size_t VERTEX_NORMAL_ATTACHMENT_ID = 1u;
		static const constexpr size_t VERTEX_NORMAL_COLOR_ATTACHMENT_ID = 2u;
		static const constexpr size_t COMPOUND_INDEX_BUFFER = 3u;
		static const constexpr size_t COLOR_ATTACHMENT_COUNT = 4u;
		static const Graphics::Texture::PixelFormat* AttachmentFormats() {
			static const Graphics::Texture::PixelFormat ATTACHMENT_FORMATS[COLOR_ATTACHMENT_COUNT] = {
				Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
				Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
				Graphics::Texture::PixelFormat::R8G8B8A8_UNORM,
				Graphics::Texture::PixelFormat::R32G32B32A32_UINT,
			};
			return ATTACHMENT_FORMATS;
		}
		static const Vector4* ClearValues() {
			static const Vector4 CLEAR_VALUES[COLOR_ATTACHMENT_COUNT] = {
				Vector4(-1.0f),
				Vector4(0.0f),
				Vector4(0.5f, 0.5f, 0.5f, 0.0f),
				Vector4(UintAsFloatBytes(~(uint32_t(0))))
			};
			return CLEAR_VALUES;
		}

		/** Instance cache and cached instance */
		class InstanceCache : public virtual ObjectCache<ObjectIdRenderer_Configuration> {
		public:
			inline static Reference<ObjectIdRenderer> GetFor(
				const ViewportDescriptor* viewport, const LayerMask& layerMask,
				const Function<Reference<StoredObject>>& createCached) {
				static InstanceCache cache;
				if (viewport == nullptr) return nullptr;
				ObjectIdRenderer_Configuration config;
				config.descriptor = viewport;
				config.layerMask = layerMask;
				return cache.GetCachedOrCreate(config, createCached);
			}
		};
#pragma warning(disable: 4250)
		class CachedInstance : public ObjectIdRenderer, public virtual InstanceCache::StoredObject {
		public:
			inline CachedInstance(
				const ViewportDescriptor* viewport, LayerMask layers,
				GraphicsObjectPipelines* pipelines,
				Graphics::BindingPool* bindingPool,
				const ModelBindingSets& modelBindingSets,
				Object* perObjectBindingProvider,
				const Graphics::ResourceBinding<Graphics::Buffer>* viewportBuffer,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* indirectionBuffer)
				: ObjectIdRenderer(
					viewport, layers, pipelines, bindingPool,
					modelBindingSets, perObjectBindingProvider,
					viewportBuffer, indirectionBuffer) {}
			inline virtual ~CachedInstance() {}
		};
#pragma warning(default: 4250)
	};
}

namespace Jimara {
	/** ObjectIdRenderer implementation */
	Reference<ObjectIdRenderer> ObjectIdRenderer::GetFor(const ViewportDescriptor* viewport, LayerMask layers, bool cached) {
		if (viewport == nullptr) return nullptr;
		
		auto fail = [&](const auto&... message) {
			viewport->Context()->Log()->Error("ObjectIdRenderer::GetFor - ", message...);
			return nullptr;
		};

		auto createNewInstance = [&](const auto& instantiate) -> Reference<ObjectIdRenderer> {
			// Get render pass:
			const Reference<Graphics::RenderPass> renderPass = viewport->Context()->Graphics()->Device()->GetRenderPass(
				Graphics::Texture::Multisampling::SAMPLE_COUNT_1,
				Helpers::COLOR_ATTACHMENT_COUNT, Helpers::AttachmentFormats(),
				viewport->Context()->Graphics()->Device()->GetDepthFormat(),
				Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH);
			if (renderPass == nullptr)
				return fail("Failed to get/create render pass! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Get graphics object set:
			const Reference<GraphicsObjectDescriptor::Set> graphicsObjects = GraphicsObjectDescriptor::Set::GetInstance(viewport->Context());
			if (graphicsObjects == nullptr)
				return fail("Failed to get GraphicsObjectDescriptor::Set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Get handle of custom viewport data source:
			Reference<GraphicsObjectPipelines::CustomViewportDataProvider> descriptorSource = [&]() {
				IndexedGraphicsObjectDataProvider::Descriptor desc = {};
				desc.graphicsObjects = graphicsObjects;
				desc.frustrumDescriptor = viewport;
				desc.customIndexBindingName = "jimara_ObjectIdRenderer_IndirectObjectIdBuffer";
				return IndexedGraphicsObjectDataProvider::GetFor(desc);
				}();
			if (descriptorSource == nullptr)
				return fail("Failed to get/create CustomViewportDataProvider! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Get GraphicsObjectPipelines:
			const Reference<GraphicsObjectPipelines> pipelines = GraphicsObjectPipelines::Get([&]() {
				GraphicsObjectPipelines::Descriptor desc = {};
				{
					desc.descriptorSet = graphicsObjects;
					desc.frustrumDescriptor = viewport;
					desc.customViewportDataProvider = descriptorSource;
					desc.renderPass = renderPass;
					desc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
					desc.layers = layers;
					desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/ObjectIdRenderer/Jimara_ObjectIdRenderer.jlm");
					desc.lightingModelStage = "Main";
				}
				return desc;
				}());
			if (pipelines == nullptr)
				return fail("Failed to get GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Create binding pool:
			const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(
				graphicsObjects->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
			if (bindingPool == nullptr)
				return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			ModelBindingSets lightingModelBindings;

			// Create bindless bindings:
			{
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = pipelines->EnvironmentPipeline();

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>> jimara_BindlessTextures =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>(
						viewport->Context()->Graphics()->Bindless().SamplerBinding());
				auto findBindlessTextures = [&](const auto&) { return jimara_BindlessTextures; };
				desc.find.bindlessTextureSamplers = &findBindlessTextures;

				const Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>> jimara_BindlessBuffers =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>(
						viewport->Context()->Graphics()->Bindless().BufferBinding());
				auto findBindlessArrays = [&](const auto&) { return jimara_BindlessBuffers; };
				desc.find.bindlessStructuredBuffers = &findBindlessArrays;

				const size_t bindlessBindingCount = Math::Max(desc.pipeline->BindingSetCount(), size_t(1u)) - 1u;
				for (size_t i = 0u; i < bindlessBindingCount; i++) {
					desc.bindingSetId = i;
					const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
					if (set == nullptr)
						return fail("Failed to allocate bindless binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					lightingModelBindings.Push(set);
				}
			}

			// Create viewport buffers:
			const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> viewportBuffer =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(
					viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>());
			if (viewportBuffer->BoundObject() == nullptr)
				return fail("Failed to allocate viewport buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> indirectionBuffer =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();

			// Create viewport binding set:
			{
				Graphics::BindingSet::Descriptor desc = {};
				desc.pipeline = pipelines->EnvironmentPipeline();
				if (desc.pipeline->BindingSetCount() <= 0u)
					return fail("Environment pipeline expected to have at least 1 binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				desc.bindingSetId = desc.pipeline->BindingSetCount() - 1u;

				auto findConstantBuffers = [&](const auto& id) -> const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> {
					if (id.name == "jimara_ObjectIdRenderer_ViewportBuffer")
						return viewportBuffer;
					else return nullptr;
					};
				desc.find.constantBuffer = &findConstantBuffers;

				auto findStructuredBuffers = [&](const auto& id) -> const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
					if (id.name == "jimara_ObjectIdRenderer_IndirectionBuffer" ||
						id.name == "jimara_LightDataBinding")
						return indirectionBuffer;
					else return nullptr;
					};
				desc.find.structuredBuffer = &findStructuredBuffers;

				const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
				if (set == nullptr)
					return fail("Failed allocate viewport binding set![File:", __FILE__, "; Line: ", __LINE__, "]");
				lightingModelBindings.Push(set);
			}

			// Create shared or non-shared instance:
			return instantiate(
				pipelines, 
				bindingPool, lightingModelBindings,
				descriptorSource,
				viewportBuffer, indirectionBuffer);
		};
		
		if (cached) {
			auto createCached = [&]() -> Reference<Helpers::InstanceCache::StoredObject> {
				return createNewInstance([&](const auto&... args) {
					return Object::Instantiate<Helpers::CachedInstance>(viewport, layers, args...);
					});
			};
			return Helpers::InstanceCache::GetFor(viewport, layers,
				Function<Reference<Helpers::InstanceCache::StoredObject>>::FromCall(&createCached));
		}
		else return createNewInstance([&](const auto&... args) -> Reference<ObjectIdRenderer> {
			Reference<ObjectIdRenderer> result = new ObjectIdRenderer(viewport, layers, args...);
			result->ReleaseRef();
			return result;
			});
	}

	void ObjectIdRenderer::SetResolution(Size2 resolution) {
		if (resolution.x <= 0) resolution.x = 1;
		if (resolution.y <= 0) resolution.y = 1;
		std::unique_lock<std::shared_mutex> lock(m_updateLock);
		m_resolution = resolution;
	}

	ObjectIdRenderer::Reader::Reader(const ObjectIdRenderer* renderer) 
		: m_renderer(renderer), m_readLock(renderer->m_updateLock) {}

	ObjectIdRenderer::ResultBuffers ObjectIdRenderer::Reader::LastResults()const {
		return m_renderer->m_buffers;
	}

	uint32_t ObjectIdRenderer::Reader::DescriptorCount()const {
		return static_cast<uint32_t>(m_renderer->m_descriptors.size());
	}

	ViewportGraphicsObjectSet::ObjectInfo ObjectIdRenderer::Reader::Descriptor(uint32_t objectIndex)const {
		if (objectIndex < m_renderer->m_descriptors.size()) {
			const auto& pair = m_renderer->m_descriptors[objectIndex];
			return ViewportGraphicsObjectSet::ObjectInfo{ pair.first.operator->(), pair.second.operator->() };
		}
		else return {};
	}

	void ObjectIdRenderer::Execute() {
		std::unique_lock<std::shared_mutex> updateLock(m_updateLock);
		{
			uint64_t curFrame = m_viewport->Context()->FrameIndex();
			if (curFrame == m_lastFrame) 
				return; //Already rendered...
			else m_lastFrame = curFrame;
		}

		if (!UpdateBuffers()) {
			m_viewport->Context()->Log()->Error(
				"ObjectIdRenderer::Execute - Failed to prepare command buffers! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return;
		}

		const GraphicsObjectPipelines::Reader reader(*m_graphicsObjectPipelines);
		const size_t pipelineCount = reader.Count();

		// Update indirection buffer:
		{
			uint32_t maxId = 0u;
			bool indirectionDataDirty = false;

			// Iterate over indirection indices, record range and update m_indirectionData:
			for (size_t i = 0u; i < pipelineCount; i++) {
				uint32_t indirectIndex = 0u;
				{
					const auto& objectInfo = reader[i];
					const IndexedGraphicsObjectDataProvider::ViewportData* data = dynamic_cast<const IndexedGraphicsObjectDataProvider::ViewportData*>(objectInfo.ViewData());
					if (data == nullptr)
						m_viewport->Context()->Log()->Error(
							"ObjectIdRenderer::Execute - Viewport data expected to be of a custom type! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					else indirectIndex = data->Index();
				}
				if (indirectIndex >= m_indirectionData.size()) {
					indirectionDataDirty = true;
					while (m_indirectionData.size() <= indirectIndex)
						m_indirectionData.push_back(0u);
				}
				if (m_indirectionData[indirectIndex] != i) {
					indirectionDataDirty = true;
					m_indirectionData[indirectIndex] = static_cast<uint32_t>(i);
				}
				maxId = Math::Max(maxId, indirectIndex);
			}

			// In case indirection data got larger, it's kind-of benefitial to keep it some power of 2 to avoid extra allocations creeping-up:
			{
				size_t indirectionSize = Math::Max(m_indirectionData.size(), size_t(1u));
				while (indirectionSize <= maxId)
					indirectionSize <<= 1u;
				if (indirectionSize > m_indirectionData.size()) {
					indirectionDataDirty = true;
					while (m_indirectionData.size() <= indirectionSize)
						m_indirectionData.push_back(0u);
				}
			}

			// Reallocate indirection buffer if it's not large enough:
			if (m_indirectionBuffer->BoundObject() == nullptr || m_indirectionBuffer->BoundObject()->ObjectCount() < m_indirectionData.size()) {
				indirectionDataDirty = true;
				m_indirectionBuffer->BoundObject() = m_viewport->Context()->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
					m_indirectionData.size(), Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
				if (m_indirectionBuffer->BoundObject() == nullptr) {
					m_viewport->Context()->Log()->Error(
						"ObjectIdRenderer::Execute - Could not allocate indirection buffer! ",
						"[File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
			}

			// Update indirection buffer data if it's dirty:
			if (indirectionDataDirty) {
				std::memcpy(m_indirectionBuffer->BoundObject()->Map(), (const void*)m_indirectionData.data(), m_indirectionData.size() * sizeof(uint32_t));
				m_indirectionBuffer->BoundObject()->Unmap(true);
			}
		}

		// Obtain command buffer:
		const Graphics::InFlightBufferInfo commandBuffer = m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();

		// Update viewport buffer:
		{
			ViewportBuffer_t& buffer = *reinterpret_cast<ViewportBuffer_t*>(m_viewportBuffer->BoundObject()->Map());
			buffer.view = m_viewport->ViewMatrix();
			buffer.projection = m_viewport->ProjectionMatrix();
			buffer.viewPose = Math::Inverse(buffer.view);
			m_viewportBuffer->BoundObject()->Unmap(true);
		}

		// Start render pass:
		m_graphicsObjectPipelines->RenderPass()->BeginPass(commandBuffer, m_buffers.frameBuffer, Helpers::ClearValues());

		// Update and bind bindless buffers:
		for (size_t i = 0u; i < m_modelBindingSets.Size(); i++) {
			Graphics::BindingSet* const set = m_modelBindingSets[i];
			set->Update(commandBuffer);
			set->Bind(commandBuffer);
		}

		// Render pipelines:
		m_descriptors.clear();
		for (size_t i = 0u; i < pipelineCount; i++) {
			const auto& objectInfo = reader[i];
			objectInfo.ExecutePipeline(commandBuffer);
			m_descriptors.push_back(DescriptorInfo(objectInfo.Descriptor(), objectInfo.ViewData()));
		}

		// End render pass:
		m_graphicsObjectPipelines->RenderPass()->EndPass(commandBuffer);
	}

	void ObjectIdRenderer::CollectDependencies(Callback<Job*> addDependency) {
		m_graphicsObjectPipelines->GetUpdateTasks(addDependency);
		m_graphicsSimulation->CollectDependencies(addDependency);
	}

	ObjectIdRenderer::ObjectIdRenderer(
		const ViewportDescriptor* viewport, LayerMask layers,
		GraphicsObjectPipelines* pipelines,
		Graphics::BindingPool* bindingPool,
		const ModelBindingSets& modelBindingSets,
		Object* perObjectBindingProvider,
		const Graphics::ResourceBinding<Graphics::Buffer>* viewportBuffer,
		Graphics::ResourceBinding<Graphics::ArrayBuffer>* indirectionBuffer)
		: m_viewport(viewport), m_layerMask(layers)
		, m_graphicsObjectPipelines(pipelines)
		, m_bindingPool(bindingPool)
		, m_modelBindingSets(modelBindingSets)
		, m_perObjectBindingProvider(perObjectBindingProvider)
		, m_viewportBuffer(viewportBuffer)
		, m_indirectionBuffer(indirectionBuffer)
		, m_graphicsSimulation(GraphicsSimulation::JobDependencies::For(viewport->Context())) {
		assert(m_viewport != nullptr);
		assert(m_graphicsObjectPipelines != nullptr);
		assert(m_bindingPool != nullptr);
		assert(m_perObjectBindingProvider != nullptr);
		assert(m_viewportBuffer != nullptr);
		assert(m_graphicsSimulation != nullptr);
	}

	ObjectIdRenderer::~ObjectIdRenderer() {}

	bool ObjectIdRenderer::UpdateBuffers() {
		const Size3 size = Size3(m_resolution, 1);
		if (m_buffers.vertexPosition != nullptr && m_buffers.compoundIndex->TargetView()->TargetTexture()->Size() == size) return true;

		TargetBuffers buffers;

		// Create new textures:
		Reference<Graphics::TextureView> colorAttachments[Helpers::COLOR_ATTACHMENT_COUNT];
		auto createTextureView = [&](Graphics::Texture::PixelFormat pixelFormat, const char* name) -> Reference<Graphics::TextureSampler> {
			Reference<Graphics::Texture> texture = m_viewport->Context()->Graphics()->Device()->CreateMultisampledTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, pixelFormat, size, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
			if (texture == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create ", name, " texture! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (view == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create TextureView for ", name, " texture! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			Reference<Graphics::TextureSampler> sampler = view->CreateSampler(Graphics::TextureSampler::FilteringMode::NEAREST);
			if (sampler == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create TextureSampler for ", name, " texture! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			return sampler;
		};
		auto createTexture = [&](size_t colorAttachmentId, const char* name) -> Reference<Graphics::TextureSampler> {
			Reference<Graphics::TextureSampler> sampler = createTextureView(Helpers::AttachmentFormats()[colorAttachmentId], name);
			if (sampler != nullptr)
				colorAttachments[colorAttachmentId] = sampler->TargetView();
			return sampler;
		};
		buffers.vertexPosition = createTexture(Helpers::VERTEX_POSITION_ATTACHMENT_ID, "vertexPosition");
		buffers.vertexNormal = createTexture(Helpers::VERTEX_NORMAL_ATTACHMENT_ID, "vertexNormal");
		buffers.vertexNormalColor = createTexture(Helpers::VERTEX_NORMAL_COLOR_ATTACHMENT_ID, "vertexNormalColor");
		buffers.compoundIndex = createTexture(Helpers::COMPOUND_INDEX_BUFFER, "compoundIndex");
		buffers.depthAttachment = createTextureView(m_viewport->Context()->Graphics()->Device()->GetDepthFormat(), "depthAttachment");
		for (size_t i = 0; i < Helpers::COLOR_ATTACHMENT_COUNT; i++)
			if (colorAttachments[i] == nullptr) 
				return false;
		if (buffers.depthAttachment == nullptr)
			return false;

		// Create frame buffer:
		buffers.frameBuffer = m_graphicsObjectPipelines->RenderPass()->CreateFrameBuffer(
			colorAttachments, buffers.depthAttachment->TargetView(), nullptr, nullptr);
		if (buffers.frameBuffer == nullptr) {
			m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create frame buffer! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return false;
		}

		// Yep, if we got here, we succeeded...
		m_buffers = buffers;
		return true;
	}
}
