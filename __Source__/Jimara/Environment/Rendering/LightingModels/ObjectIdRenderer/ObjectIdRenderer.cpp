#include "ObjectIdRenderer.h"


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
		static const constexpr size_t OBJECT_INDEX_ATTACHMENT_ID = 2u;
		static const constexpr size_t INSTANCE_INDEX_ATTACHMENT_ID = 3u;
		static const constexpr size_t PRIMITIVE_INDEX_ATTACHMENT_ID = 4u;
		static const constexpr size_t VERTEX_NORMAL_COLOR_ATTACHMENT_ID = 5u;
		static const constexpr size_t COLOR_ATTACHMENT_COUNT = 6u;
		static const Graphics::Texture::PixelFormat* AttachmentFormats() {
			static const Graphics::Texture::PixelFormat ATTACHMENT_FORMATS[COLOR_ATTACHMENT_COUNT] = {
				Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
				Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
				Graphics::Texture::PixelFormat::R32_UINT,
				Graphics::Texture::PixelFormat::R32_UINT,
				Graphics::Texture::PixelFormat::R32_UINT,
				Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
			};
			return ATTACHMENT_FORMATS;
		}
		static const Vector4* ClearValues() {
			static const Vector4 CLEAR_VALUES[COLOR_ATTACHMENT_COUNT] = {
				Vector4(-1.0f),
				Vector4(0.0f),
				Vector4(UintAsFloatBytes(~(uint32_t(0)))),
				Vector4(UintAsFloatBytes(~(uint32_t(0)))),
				Vector4(UintAsFloatBytes(~(uint32_t(0)))),
				Vector4(0.5f, 0.5f, 0.5f, 0.0f)
			};
			return CLEAR_VALUES;
		}



		/** Shared object id buffers: */
		class SharedObjectIdBuffers : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<Graphics::GraphicsDevice> m_device;
			const Reference<OS::Logger> m_log;

			std::shared_mutex m_lock;
			std::vector<Reference<const Graphics::ResourceBinding<Graphics::Buffer>>> m_buffers;

		public:
			inline SharedObjectIdBuffers(Graphics::GraphicsDevice* device, OS::Logger* logger) 
				: m_device(device), m_log(logger == nullptr ? device->Log() : logger) {
				assert(m_device != nullptr);
				assert(m_log != nullptr);
			}

			inline virtual ~SharedObjectIdBuffers() {}

			inline static Reference<SharedObjectIdBuffers> GetFor(Graphics::GraphicsDevice* device, OS::Logger* logger) {
				struct Cache : public virtual ObjectCache<Reference<const Object>> {
					static Reference<SharedObjectIdBuffers> Get(Graphics::GraphicsDevice* dev, OS::Logger* log) {
						static Cache cache;
						return cache.GetCachedOrCreate(dev, false, [&]() { return Object::Instantiate<SharedObjectIdBuffers>(dev, log); });
					}
				};
				return Cache::Get(device, logger);
			}

			inline bool RequireCount(size_t count) {
				std::unique_lock<std::shared_mutex> lock(m_lock);
				while (m_buffers.size() < count) {
					const uint32_t bufferId = static_cast<uint32_t>(m_buffers.size());
					if (bufferId != m_buffers.size()) {
						m_log->Error("ObjectIdRenderer::Helpers::SharedObjectIdBuffers::RequireCount - ",
							"Buffer id overflow ", m_buffers.size(), "! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					const Graphics::BufferReference<uint32_t> buffer = m_device->CreateConstantBuffer<uint32_t>();
					if (buffer == nullptr) {
						m_log->Error("ObjectIdRenderer::Helpers::SharedObjectIdBuffers::RequireCount - ", 
							"Failed to allocate constant buffer for index ", m_buffers.size(), "! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					buffer.Map() = bufferId;
					buffer->Unmap(true);
					m_buffers.push_back(Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(buffer));
				}
				return true;
			}

			template<typename ReportFn>
			inline static bool GetRange(Object* buffers, size_t first, size_t count, const ReportFn& report) {
				SharedObjectIdBuffers* self = dynamic_cast<SharedObjectIdBuffers*>(buffers);
				if (self == nullptr)
					return false;
				if (!self->RequireCount(first + count))
					return false;
				std::shared_lock<std::shared_mutex> lock(self->m_lock);
				for (size_t i = 0u; i < count; i++)
					if (!report(self->m_buffers[i + first].operator->()))
						return false;
				return true;
			}
		};


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
				return cache.GetCachedOrCreate(config, false, createCached);
			}
		};
#pragma warning(disable: 4250)
		class CachedInstance : public ObjectIdRenderer, public virtual InstanceCache::StoredObject {
		public:
			inline CachedInstance(
				const ViewportDescriptor* viewport, LayerMask layers,
				GraphicsObjectPipelines* pipelines,
				Graphics::BindingPool* bindingPool,
				const BindlessBindings& bindlessBindings,
				Object* objectIdBindings,
				const Graphics::ResourceBinding<Graphics::ArrayBuffer>* viewportBuffer,
				const Graphics::ArrayBufferReference<ViewportBuffer_t>& stagingViewportBuffers)
				: ObjectIdRenderer(
					viewport, layers, pipelines, bindingPool,
					bindlessBindings, objectIdBindings, viewportBuffer, stagingViewportBuffers) {}
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

			// Get GraphicsObjectPipelines:
			const Reference<GraphicsObjectPipelines> pipelines = GraphicsObjectPipelines::Get([&]() {
				GraphicsObjectPipelines::Descriptor desc = {};
				{
					desc.descriptorSet = graphicsObjects;
					desc.viewportDescriptor = viewport;
					desc.renderPass = renderPass;
					desc.flags = GraphicsObjectPipelines::Flags::DISABLE_ALPHA_BLENDING;
					desc.layers = layers;
					desc.lightingModel = OS::Path("Jimara/Environment/Rendering/LightingModels/ObjectIdRenderer/Jimara_ObjectIdRenderer.jlm");
				}
				return desc;
				}());
			if (pipelines == nullptr)
				return fail("Failed to get GraphicsObjectPipelines! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Create binding pool:
			const Reference<Graphics::BindingPool> bindingPool = viewport->Context()->Graphics()->Device()->CreateBindingPool(1u);
			if (bindingPool == nullptr)
				return fail("Failed to create binding pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Create bindless bindings:
			BindlessBindings bindlessBindings;
			const size_t inFlightCommandBufferCount = viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount();
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
					bindlessBindings.Push({});
					BindlessBindingSet& sets = bindlessBindings[bindlessBindings.Size() - 1u];
					for (size_t j = 0u; j < inFlightCommandBufferCount; j++) {
						const Reference<Graphics::BindingSet> set = bindingPool->AllocateBindingSet(desc);
						if (set == nullptr)
							return fail("Failed to allocate bindless binding set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						sets.Push(set);
					}
				}
			}

			// Get handle of shared object id buffers:
			const Reference<Helpers::SharedObjectIdBuffers> objectIdBuffers = Helpers::SharedObjectIdBuffers::GetFor(
				viewport->Context()->Graphics()->Device(), viewport->Context()->Log());
			if (objectIdBuffers == nullptr)
				return fail("Failed to get/create SharedObjectIdBuffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Create viewport buffers:
			const Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> viewportBuffer =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>(
					viewport->Context()->Graphics()->Device()->CreateArrayBuffer<ViewportBuffer_t>(1u, Graphics::ArrayBuffer::CPUAccess::CPU_WRITE_ONLY));
			if (viewportBuffer->BoundObject() == nullptr)
				return fail("Failed to allocate viewport buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			Graphics::ArrayBufferReference<ViewportBuffer_t> stagingViewportBuffers = viewport->Context()->Graphics()->Device()
				->CreateArrayBuffer<ViewportBuffer_t>(inFlightCommandBufferCount, Graphics::ArrayBuffer::CPUAccess::CPU_READ_WRITE);
			if (stagingViewportBuffers == nullptr)
				return fail("Failed to allocate stagingViewportBuffers! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			// Create shared or non-shared instance:
			return instantiate(pipelines, bindingPool, bindlessBindings, objectIdBuffers, viewportBuffer, stagingViewportBuffers);
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
			// TODO: Commented out, because this whole thing has to be able to run when paused too...
			//uint64_t curFrame = m_viewport->Context()->FrameIndex();
			//if (curFrame == m_lastFrame) return; //Already rendered...
			//else m_lastFrame = curFrame;
		}

		if (!UpdateBuffers()) {
			m_viewport->Context()->Log()->Error(
				"ObjectIdRenderer::Execute - Failed to prepare command buffers! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return;
		}

		const GraphicsObjectPipelines::Reader reader(*m_graphicsObjectPipelines);
		const size_t pipelineCount = reader.Count();

		// Create objectId bindings if needed:
		if (m_lightingModelBindings.size() < pipelineCount) {
			Graphics::BindingSet::Descriptor desc = {};
			desc.pipeline = m_graphicsObjectPipelines->EnvironmentPipeline();
			if (desc.pipeline->BindingSetCount() <= 0u) {
				m_viewport->Context()->Log()->Error(
					"ObjectIdRenderer::Execute - Environment pipeline expected to have at least 1 binding set! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			desc.bindingSetId = desc.pipeline->BindingSetCount() - 1u;

			auto findStructuredBuffers = [&](const auto&) { return m_viewportBuffer; };
			desc.find.structuredBuffer = &findStructuredBuffers;

			if (!Helpers::SharedObjectIdBuffers::GetRange(
				m_objectIdBindings, m_lightingModelBindings.size(), (pipelineCount - m_lightingModelBindings.size()),
				[&](const Graphics::ResourceBinding<Graphics::Buffer>* binding) -> bool {
					auto findConstantBuffer = [&](const auto&) { return binding; };
					desc.find.constantBuffer = &findConstantBuffer;
					const Reference<Graphics::BindingSet> set = m_bindingPool->AllocateBindingSet(desc);
					if (set == nullptr) {
						m_viewport->Context()->Log()->Error(
							"ObjectIdRenderer::Execute - Failed allocate binding set for object ", m_lightingModelBindings.size(), "! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
						return false;
					}
					set->Update(0u);
					m_lightingModelBindings.push_back(set);
					return true;
				})) {
				m_viewport->Context()->Log()->Error(
					"ObjectIdRenderer::Execute - Failed to generate binding sets for each object index! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Obtain command buffer:
		Graphics::InFlightBufferInfo commandBufferInfo = m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();
		const Graphics::InFlightBufferInfo commandBuffer = Graphics::InFlightBufferInfo(commandBufferInfo.commandBuffer, 0u);

		// Update viewport buffer:
		{
			ViewportBuffer_t& buffer = m_stagingViewportBuffers.Map()[commandBufferInfo.inFlightBufferId];
			buffer.view = m_viewport->ViewMatrix();
			buffer.projection = m_viewport->ProjectionMatrix();
			buffer.viewPose = Math::Inverse(buffer.view);
			m_stagingViewportBuffers->Unmap(true);
			m_viewportBuffer->BoundObject()->Copy(commandBufferInfo, m_stagingViewportBuffers,
				sizeof(ViewportBuffer_t), 0u, sizeof(ViewportBuffer_t) * commandBufferInfo.inFlightBufferId);
		}

		// Start render pass:
		m_graphicsObjectPipelines->RenderPass()->BeginPass(commandBufferInfo, m_buffers.frameBuffer, Helpers::ClearValues());

		// Update and bind bindless buffers:
		for (size_t i = 0u; i < m_bindlessBindings.Size(); i++) {
			Graphics::BindingSet* const set = m_bindlessBindings[i][commandBufferInfo.inFlightBufferId];
			set->Update(commandBuffer);
			set->Bind(commandBuffer);
		}

		// Render pipelines:
		m_descriptors.clear();
		for (size_t i = 0u; i < pipelineCount; i++) {
			m_lightingModelBindings[i]->Bind(commandBuffer);
			const auto& objectInfo = reader[i];
			objectInfo.ExecutePipeline(commandBufferInfo);
			m_descriptors.push_back(DescriptorInfo(objectInfo.Descriptor(), objectInfo.ViewData()));
		}

		// End render pass:
		m_graphicsObjectPipelines->RenderPass()->EndPass(commandBufferInfo);
	}

	void ObjectIdRenderer::CollectDependencies(Callback<Job*> addDependency) {
		m_graphicsObjectPipelines->GetUpdateTasks(addDependency);
	}

	ObjectIdRenderer::ObjectIdRenderer(
		const ViewportDescriptor* viewport, LayerMask layers,
		GraphicsObjectPipelines* pipelines,
		Graphics::BindingPool* bindingPool,
		const BindlessBindings& bindlessBindings,
		Object* objectIdBindings,
		const Graphics::ResourceBinding<Graphics::ArrayBuffer>* viewportBuffer,
		const Graphics::ArrayBufferReference<ViewportBuffer_t>& stagingViewportBuffers) 
		: m_viewport(viewport), m_layerMask(layers)
		, m_graphicsObjectPipelines(pipelines)
		, m_bindingPool(bindingPool)
		, m_bindlessBindings(bindlessBindings)
		, m_objectIdBindings(objectIdBindings)
		, m_viewportBuffer(viewportBuffer)
		, m_stagingViewportBuffers(stagingViewportBuffers) {
		assert(m_viewport != nullptr);
		assert(m_graphicsObjectPipelines != nullptr);
		assert(m_bindingPool != nullptr);
		assert(m_objectIdBindings != nullptr);
		assert(m_viewportBuffer != nullptr);
		assert(m_stagingViewportBuffers != nullptr);
	}

	ObjectIdRenderer::~ObjectIdRenderer() {}

	bool ObjectIdRenderer::UpdateBuffers() {
		const Size3 size = Size3(m_resolution, 1);
		if (m_buffers.vertexPosition != nullptr && m_buffers.instanceIndex->TargetView()->TargetTexture()->Size() == size) return true;

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
		buffers.objectIndex = createTexture(Helpers::OBJECT_INDEX_ATTACHMENT_ID, "objectIndex");
		buffers.instanceIndex = createTexture(Helpers::INSTANCE_INDEX_ATTACHMENT_ID, "instanceIndex");
		buffers.primitiveIndex = createTexture(Helpers::PRIMITIVE_INDEX_ATTACHMENT_ID, "primitiveIndex");
		buffers.vertexNormalColor = createTexture(Helpers::VERTEX_NORMAL_COLOR_ATTACHMENT_ID, "vertexNormalColor");
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
