#include "ObjectIdRenderer.h"
#include "../GraphicsEnvironment.h"


namespace Jimara {
	namespace {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ObjectIdRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_ObjectIdRenderer_ViewportBuffer");

		public:
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_ObjectIdRenderer_ViewportBuffer->BindingName()) return jimara_ObjectIdRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override {
				return nullptr;
			}

			static const EnvironmentShapeDescriptor& Instance() {
				static EnvironmentShapeDescriptor instance;
				return instance;
			}
		};

		/** CONCRETE ENVIRONMENT BINDINGS: */
		class EnvironmentDescriptor : public virtual EnvironmentShapeDescriptor {
		private:
			struct ViewportBuffer_t {
				alignas(16) Matrix4 view;
				alignas(16) Matrix4 projection;
			};

			const Reference<const LightingModel::ViewportDescriptor> m_viewport;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const LightingModel::ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
				jimara_LightDataBinding->BoundObject() = viewport->Context()->Graphics()->Device()->CreateArrayBuffer(
					m_viewport->Context()->Graphics()->Configuration().PerLightDataSize(), 1);
				jimara_ObjectIdRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
			}

			inline void Update(float aspect) {
				ViewportBuffer_t& buffer = m_viewportBuffer.Map();
				buffer.view = m_viewport->ViewMatrix();
				buffer.projection = m_viewport->ProjectionMatrix(aspect);
				m_viewportBuffer->Unmap(true);
			}
		};


		/** GraphicsObjectDescriptor with index */
		class GraphicsObjectDescriptorWithId : public virtual GraphicsObjectDescriptor {
		private:
			const Reference<GraphicsObjectDescriptor> m_descriptor;
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ObjectIdRenderer_ObjectIdBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_ObjectIdRenderer_ObjectIdBuffer");
			const Graphics::BufferReference<uint32_t> m_indexBuffer;
			uint32_t m_index = 0;

		public:
			inline GraphicsObjectDescriptorWithId(GraphicsObjectDescriptor* descriptor, Graphics::GraphicsDevice* device, uint32_t index) 
				: GraphicsObjectDescriptor(descriptor->ShaderClass())
				, m_descriptor(descriptor)
				, m_indexBuffer(device->CreateConstantBuffer<uint32_t>())
				, m_index(index) {
				if (m_indexBuffer == nullptr)
					device->Log()->Fatal("ObjectIdRenderer::GraphicsObjectDescriptorWithId - Failed to create index buffer!");
				m_indexBuffer.Map() = m_index;
				m_indexBuffer->Unmap(true);
				jimara_ObjectIdRenderer_ObjectIdBuffer->BoundObject() = m_indexBuffer;
			}

			inline virtual AABB Bounds()const final override { return m_descriptor->Bounds(); }

			inline virtual size_t VertexBufferCount()const final override { return m_descriptor->VertexBufferCount(); }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const final override { return m_descriptor->VertexBuffer(index); }

			inline virtual size_t InstanceBufferCount()const final override { return m_descriptor->InstanceBufferCount(); }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const final override { return m_descriptor->InstanceBuffer(index); }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const final override { return m_descriptor->IndexBuffer(); }
			inline virtual size_t IndexCount()const final override { return m_descriptor->IndexCount(); }
			inline virtual size_t InstanceCount()const final override { return m_descriptor->InstanceCount(); }

			inline virtual Reference<Component> GetComponent(size_t instanceId, size_t primitiveId)const final override { 
				return m_descriptor->GetComponent(instanceId, primitiveId); 
			}


			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_ObjectIdRenderer_ObjectIdBuffer->BindingName()) return jimara_ObjectIdRenderer_ObjectIdBuffer;
				else return m_descriptor->FindConstantBufferBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				return m_descriptor->FindStructuredBufferBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
				return m_descriptor->FindTextureSamplerBinding(name);
			}

			void SetId(uint32_t id) {
				if (m_index == id) return;
				m_index = id;
				m_indexBuffer.Map() = m_index;
				m_indexBuffer->Unmap(true);
			}
		};


		/** Render pass constants: */
		inline static float UintAsFloatBytes(uint32_t value) { return *reinterpret_cast<float*>(&value); }
		static const Graphics::Texture::PixelFormat ATTACHMENT_FORMATS[] = {
			Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
			Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
			Graphics::Texture::PixelFormat::R32_UINT,
			Graphics::Texture::PixelFormat::R32_UINT,
			Graphics::Texture::PixelFormat::R32_UINT,
			Graphics::Texture::PixelFormat::R32G32B32A32_SFLOAT,
		};
		static const Vector4 CLEAR_VALUES[] = {
			Vector4(-1.0f),
			Vector4(0.0f),
			Vector4(UintAsFloatBytes(~(uint32_t(0)))),
			Vector4(UintAsFloatBytes(~(uint32_t(0)))),
			Vector4(0.0f),
		};
		static const size_t VERTEX_POSITION_ATTACHMENT_ID = 0;
		static const size_t VERTEX_NORMAL_ATTACHMENT_ID = 1;
		static const size_t OBJECT_INDEX_ATTACHMENT_ID = 2;
		static const size_t INSTANCE_INDEX_ATTACHMENT_ID = 3;
		static const size_t PRIMITIVE_INDEX_ATTACHMENT_ID = 4;
		static const size_t VERTEX_NORMAL_COLOR_ATTACHMENT_ID = 5;
		static constexpr size_t ColorAttachmentCount() { return sizeof(ATTACHMENT_FORMATS) / sizeof(Graphics::Texture::PixelFormat); }


		/** GraphicsObjectDescriptor TO Graphics::GraphicsPipeline::Descriptor TRANSLATION PER GraphicsContext INSTANCE: */
		
		struct PipelineDescPerObject {
			Reference<GraphicsObjectDescriptor> sceneObject;
			mutable Reference<GraphicsObjectDescriptorWithId> objectWithId;
			mutable Reference<Graphics::GraphicsPipeline::Descriptor> descriptor;

			inline PipelineDescPerObject(GraphicsObjectDescriptor* obj = nullptr) : sceneObject(obj) {}
		};

		class PipelineObjects : public virtual ObjectCache<Reference<Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<Graphics::ShaderSet> m_shaderSet;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjects;
			const Reference<Graphics::RenderPass> m_renderPass;
			std::shared_mutex mutable m_dataLock;
			Reference<GraphicsEnvironment> m_environment;
			ObjectSet<GraphicsObjectDescriptor, PipelineDescPerObject> m_activeObjects;
			//Reference<Graphics::GraphicsPipelineSet> m_pipelineSet;
			EventInstance<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t> m_onPipelinesAdded;
			EventInstance<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t> m_onPipelinesRemoved;
			ThreadBlock m_descriptorCreationBlock;

			template<typename ChangeCallback>
			inline void UpdateSet(const PipelineDescPerObject* objects, size_t count, const ChangeCallback& changeCallback) {
				static thread_local std::vector<Reference<Graphics::GraphicsPipeline::Descriptor>> descriptors;
				descriptors.resize(count);
				for (size_t i = 0; i < count; i++) descriptors[i] = objects[i].descriptor;
				changeCallback(descriptors.data(), descriptors.size());
				descriptors.clear();
			}

			inline void OnObjectsAddedLockless(GraphicsObjectDescriptor* const* objects, size_t count) {
				if (count <= 0) return;

				// Create environment if it does not exist:
				if (m_environment == nullptr) {
					for (size_t i = 0; i < count; i++) {
						GraphicsObjectDescriptor* sampleObject = objects[i];
						if (sampleObject != nullptr) {
							m_environment = GraphicsEnvironment::Create(m_shaderSet, EnvironmentShapeDescriptor::Instance(), sampleObject, m_context->Graphics()->Device());
							if (m_environment != nullptr) break;
						}
					}
				}
				if (m_environment == nullptr) return;

				// Add new objects and create pipeline descriptors:
				m_activeObjects.Add(objects, count, [&](const PipelineDescPerObject* added, size_t numAdded) {
#ifndef NDEBUG
					if (numAdded != count) m_context->Log()->Error("ObjectIdRenderer::PipelineObjects::OnObjectsAddedLockless - (numAdded != count)!");
#endif
					static const size_t MAX_THREADS = max(std::thread::hardware_concurrency(), 1u);
					const size_t MIN_OBJECTS_PER_THREAD = 32;
					const size_t threads = min((numAdded + MIN_OBJECTS_PER_THREAD - 1) / MIN_OBJECTS_PER_THREAD, MAX_THREADS);
					
					std::pair<const PipelineDescPerObject*, size_t> changeInfo(added, numAdded);
					void(*createFn)(std::pair<const PipelineDescPerObject*, size_t>*, ThreadBlock::ThreadInfo, void*) =
						[](std::pair<const PipelineDescPerObject*, size_t>* change, ThreadBlock::ThreadInfo thread, void* selfPtr) {
						PipelineObjects* self = reinterpret_cast<PipelineObjects*>(selfPtr);
						const PipelineDescPerObject* end = (change->first + change->second);
						for (const PipelineDescPerObject* ptr = change->first + thread.threadId; ptr < end; ptr += thread.threadCount) {
							if (ptr->sceneObject->ShaderClass() == nullptr) continue;
							ptr->objectWithId = Object::Instantiate<GraphicsObjectDescriptorWithId>(
								ptr->sceneObject, self->m_context->Graphics()->Device(), (uint32_t)(ptr - self->m_activeObjects.Data()));
							ptr->descriptor = self->m_environment->CreateGraphicsPipelineDescriptor(ptr->objectWithId);
							if (ptr->descriptor == nullptr) self->m_context->Log()->Error(
								"ObjectIdRenderer::PipelineObjects::OnObjectsAddedLockless - Failed to create graphics pipeline descriptor!");
						}
					};
					Callback<ThreadBlock::ThreadInfo, void*> createCall(createFn, &changeInfo);

					if (threads <= 1) {
						ThreadBlock::ThreadInfo info;
						info.threadCount = 1;
						info.threadId = 0;
						createCall(info, this);
					}
					else m_descriptorCreationBlock.Execute(threads, this, createCall);

					UpdateSet(added, numAdded, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) {
						//m_pipelineSet->AddPipelines(descs, numDescs);
						m_onPipelinesAdded(descs, numDescs);
						});
					});
			}

			inline void OnObjectsRemovedLockless(GraphicsObjectDescriptor* const* objects, size_t count) {
				if (count <= 0) return;
				m_activeObjects.Remove(objects, count, [&](const PipelineDescPerObject* removed, size_t numRemoved) {
#ifndef NDEBUG
					if (numRemoved != count) m_context->Log()->Error("ObjectIdRenderer::PipelineObjects::OnObjectsRemovedLockless - (numRemoved != count)!");
#endif
					for (size_t i = 0; i < m_activeObjects.Size(); i++)
						m_activeObjects[i].objectWithId->SetId((uint32_t)i);
					UpdateSet(removed, numRemoved, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) {
						//m_pipelineSet->RemovePipelines(descs, numDescs);
						m_onPipelinesRemoved(descs, numDescs);
						});
					});
			}

			inline void OnObjectsAdded(GraphicsObjectDescriptor* const* objects, size_t count) {
				std::unique_lock<std::shared_mutex> lock(m_dataLock);
				OnObjectsAddedLockless(objects, count);
			}

			inline void OnObjectsRemoved(GraphicsObjectDescriptor* const* objects, size_t count) {
				std::unique_lock<std::shared_mutex> lock(m_dataLock);
				OnObjectsRemovedLockless(objects, count);
			}

		public:
			inline PipelineObjects(SceneContext* context)
				: m_context(context)
				, m_shaderSet(context->Graphics()->Configuration().ShaderLoader()
					->LoadShaderSet("Jimara/Environment/GraphicsContext/LightingModels/ObjectIdRenderer/Jimara_ObjectIdRenderer.jlm"))
				, m_graphicsObjects(GraphicsObjectDescriptor::Set::GetInstance(context))
				, m_renderPass(context->Graphics()->Device()->CreateRenderPass(
					Graphics::Texture::Multisampling::SAMPLE_COUNT_1,
					ColorAttachmentCount(), ATTACHMENT_FORMATS,
					context->Graphics()->Device()->GetDepthFormat(), false)) {
				if (m_renderPass == nullptr)
					m_context->Log()->Fatal("ObjectIdRenderer::PipelineObjects - Failed to create render pass!");
				/*
				m_pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(
					m_context->Graphics()->Device()->GraphicsQueue(), m_renderPass,
					m_context->Graphics()->Configuration().MaxInFlightCommandBufferCount(),
					max(std::thread::hardware_concurrency() / 2, 1u));
				if (m_pipelineSet == nullptr)
					m_context->Log()->Fatal("ObjectIdRenderer::PipelineObjects - Failed to create the pipeline set!");
				*/
				
				m_graphicsObjects->OnAdded() += Callback(&PipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() += Callback(&PipelineObjects::OnObjectsRemoved, this);

				std::unique_lock<std::shared_mutex> lock(m_dataLock);
				std::vector<GraphicsObjectDescriptor*> descriptors;
				m_graphicsObjects->GetAll([&](GraphicsObjectDescriptor* descriptor) { 
					descriptors.push_back(descriptor);
					descriptor->AddRef();
					});
				OnObjectsAddedLockless(descriptors.data(), descriptors.size());
				for (size_t i = 0; i < descriptors.size(); i++)
					descriptors[i]->ReleaseRef();
			}

			inline virtual ~PipelineObjects() {
				m_graphicsObjects->OnAdded() -= Callback(&PipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() -= Callback(&PipelineObjects::OnObjectsRemoved, this);
			}

			Graphics::RenderPass* RenderPass()const { return m_renderPass; }

			class Cache : public virtual ObjectCache<Reference<Object>> {
			public:
				inline static Reference<PipelineObjects> GetObjects(SceneContext* context) {
					static Cache cache;
					return cache.GetCachedOrCreate(context, false, [&]()->Reference<PipelineObjects> { return Object::Instantiate<PipelineObjects>(context); });
				};
			};

			class Reader : public virtual std::shared_lock<std::shared_mutex> {
			private:
				const Reference<PipelineObjects> m_objects;

			public:
				inline Reader(PipelineObjects* objects)
					: std::shared_lock<std::shared_mutex>(objects->m_dataLock), m_objects(objects) {}
				inline void GetDescriptorData(const PipelineDescPerObject*& data, size_t& count) {
					data = m_objects->m_activeObjects.Data();
					count = m_objects->m_activeObjects.Size();
				}
				inline Graphics::ShaderSet* ShaderSet()const { return m_objects->m_shaderSet; }
				//inline Graphics::GraphicsPipelineSet* PipelineSet()const { return m_objects->m_pipelineSet; }
				inline Event<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t>& OnPipelinesAdded()const { return m_objects->m_onPipelinesAdded; }
				inline Event<const Reference<Graphics::GraphicsPipeline::Descriptor>*, size_t>& OnPipelinesRemoved()const { return m_objects->m_onPipelinesRemoved; }

				inline void SubscribePipelineSet(Graphics::GraphicsPipelineSet* set) {
					OnPipelinesAdded() += Callback(&Graphics::GraphicsPipelineSet::AddPipelines, set);
					OnPipelinesAdded() += Callback(&Graphics::GraphicsPipelineSet::RemovePipelines, set);
				}
				inline void UnsubscribePipelineSet(Graphics::GraphicsPipelineSet* set) {
					OnPipelinesAdded() -= Callback(&Graphics::GraphicsPipelineSet::AddPipelines, set);
					OnPipelinesAdded() -= Callback(&Graphics::GraphicsPipelineSet::RemovePipelines, set);
				}
			};
		};

		/** Instance cache */
		class Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<ObjectIdRenderer> GetFor(const LightingModel::ViewportDescriptor* viewport) {
				static Cache cache;
				return cache.GetCachedOrCreate(viewport, false, [&]() -> Reference<ObjectIdRenderer> {
					return ObjectIdRenderer::GetFor(viewport, false);
					});
			}
		};
	}





	/** ObjectIdRenderer implementation */

	Reference<ObjectIdRenderer> ObjectIdRenderer::GetFor(const LightingModel::ViewportDescriptor* viewport, bool cached) {
		if (viewport == nullptr) return nullptr;
		else if (cached) return Cache::GetFor(viewport);
		else {
			Reference<ObjectIdRenderer> result = new ObjectIdRenderer(viewport);
			result->ReleaseRef();
			return result;
		}
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

	GraphicsObjectDescriptor* ObjectIdRenderer::Reader::Descriptor(uint32_t objectIndex)const {
		if (objectIndex < m_renderer->m_descriptors.size())
			return m_renderer->m_descriptors[objectIndex];
		else return nullptr;
	}

	namespace {
		inline static Reference<Graphics::Pipeline> CreateEnvironmentPipeline(
			Graphics::ShaderResourceBindings::ShaderResourceBindingSet& descriptor, Graphics::ShaderSet* shaderSet,
			const PipelineDescPerObject* pipelines, size_t pipelineCount, SceneContext* context) {
			if (pipelines == nullptr || pipelineCount <= 0) return nullptr;
			else for (size_t i = 0; i < pipelineCount; i++) {
				GraphicsObjectDescriptor* sampleObject = pipelines[i].objectWithId;
				if (sampleObject == nullptr) continue;
				Reference<GraphicsEnvironment> environment = GraphicsEnvironment::Create(
					shaderSet, descriptor, sampleObject, context->Graphics()->Device());
				if (environment == nullptr) continue;
				Reference<Graphics::PipelineDescriptor> descriptor = environment->EnvironmentDescriptor();
				if (descriptor == nullptr) continue;
				Reference<Graphics::Pipeline> environmentPipeline = context->Graphics()->Device()->CreateEnvironmentPipeline(
					descriptor, context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				if (environmentPipeline != nullptr)
					return environmentPipeline;
			}
			return nullptr;
		}

		inline static void CacheBuffers(
			const PipelineDescPerObject* pipelines, size_t pipelineCount,
			std::vector<Reference<GraphicsObjectDescriptor>>& descriptors) {
			descriptors.clear();
			for (size_t i = 0; i < pipelineCount; i++)
				descriptors.push_back(pipelines[i].sceneObject);
		}
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
			m_viewport->Context()->Log()->Error("ObjectIdRenderer::Execute - Failed to prepare command buffers!");
			return;
		}
		PipelineObjects* pipelineObjects = dynamic_cast<PipelineObjects*>(m_pipelineObjects.operator->());
		EnvironmentDescriptor* environmentDescriptor = dynamic_cast<EnvironmentDescriptor*>(m_environmentDescriptor.operator->());

		PipelineObjects::Reader reader(pipelineObjects);
		const PipelineDescPerObject* pipelines;
		size_t pipelineCount;
		reader.GetDescriptorData(pipelines, pipelineCount);
		if (pipelineCount <= 0 || pipelines == nullptr) return;
		else if (m_environmentPipeline == nullptr) {
			m_environmentPipeline = CreateEnvironmentPipeline(
				*m_environmentDescriptor,
				reader.ShaderSet(), pipelines, pipelineCount,
				m_viewport->Context());
			if (m_environmentPipeline == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::Execute - Failed to create the environment pipeline!");
				return;
			}
		}

		CacheBuffers(pipelines, pipelineCount, m_descriptors);

		environmentDescriptor->Update(((float)m_resolution.x) / ((float)m_resolution.y));

		Graphics::Pipeline::CommandBufferInfo commandBufferInfo = m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer();

		Graphics::PrimaryCommandBuffer* buffer = dynamic_cast<Graphics::PrimaryCommandBuffer*>(commandBufferInfo.commandBuffer);
		if (buffer == nullptr) {
			m_viewport->Context()->Log()->Error("ObjectIdRenderer::Execute - GetWorkerThreadCommandBuffer().commandBuffer should be a primary command buffer!");
			return;
		}
		pipelineObjects->RenderPass()->BeginPass(buffer, m_buffers.frameBuffer, CLEAR_VALUES, true);
		if (m_environmentPipeline != nullptr)
			m_pipelineSet->ExecutePipelines(buffer, commandBufferInfo.inFlightBufferId, m_buffers.frameBuffer, m_environmentPipeline);
		pipelineObjects->RenderPass()->EndPass(buffer);
	}

	void ObjectIdRenderer::CollectDependencies(Callback<Job*> addDependency) {
		Unused(addDependency);
	}

	ObjectIdRenderer::ObjectIdRenderer(const LightingModel::ViewportDescriptor* viewport)
		: m_viewport(viewport)
		, m_environmentDescriptor(Object::Instantiate<EnvironmentDescriptor>(viewport))
		, m_pipelineObjects(PipelineObjects::Cache::GetObjects(viewport->Context())) {
		
		std::unique_lock<std::shared_mutex> updateLock(m_updateLock);

		PipelineObjects* pipelineObjects = dynamic_cast<PipelineObjects*>(m_pipelineObjects.operator->());
		PipelineObjects::Reader reader(pipelineObjects);
		
		m_pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(
			m_viewport->Context()->Graphics()->Device()->GraphicsQueue(),
			pipelineObjects->RenderPass(),
			m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(),
			max(std::thread::hardware_concurrency() / 2, 1u));
		if (m_pipelineSet == nullptr)
			m_viewport->Context()->Log()->Fatal("ObjectIdRenderer::ObjectIdRenderer - Failed to create the pipeline set!");

		{
			const PipelineDescPerObject* pipelines;
			size_t pipelineCount;
			reader.GetDescriptorData(pipelines, pipelineCount);
			if (pipelineCount > 0) {
				static thread_local std::vector<Reference<Graphics::GraphicsPipeline::Descriptor>> descriptors;
				descriptors.clear();
				for (size_t i = 0; i < pipelineCount; i++) {
					Graphics::GraphicsPipeline::Descriptor* descriptor = pipelines[i].descriptor;
					if (descriptor != nullptr)
						descriptors.push_back(descriptor);
				}
				m_pipelineSet->AddPipelines(descriptors.data(), descriptors.size());
				descriptors.clear();
			}
		}

		reader.SubscribePipelineSet(m_pipelineSet);
	}

	ObjectIdRenderer::~ObjectIdRenderer() {
		PipelineObjects* pipelineObjects = dynamic_cast<PipelineObjects*>(m_pipelineObjects.operator->());
		PipelineObjects::Reader reader(pipelineObjects);
		reader.UnsubscribePipelineSet(m_pipelineSet);
	}

	bool ObjectIdRenderer::UpdateBuffers() {
		const Size3 size = Size3(m_resolution, 1);
		if (m_buffers.vertexPosition != nullptr && m_buffers.instanceIndex->TargetView()->TargetTexture()->Size() == size) return true;

		TargetBuffers buffers;

		// Create new textures:
		Reference<Graphics::TextureView> colorAttachments[ColorAttachmentCount()];
		auto createTextureView = [&](Graphics::Texture::PixelFormat pixelFormat, const char* name) -> Reference<Graphics::TextureSampler> {
			Reference<Graphics::Texture> texture = m_viewport->Context()->Graphics()->Device()->CreateMultisampledTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, pixelFormat, size, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
			if (texture == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create ", name, " texture!");
				return nullptr;
			}
			Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (view == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create TextureView for ", name, " texture!");
				return nullptr;
			}
			Reference<Graphics::TextureSampler> sampler = view->CreateSampler(Graphics::TextureSampler::FilteringMode::NEAREST);
			if (sampler == nullptr) {
				m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create TextureSampler for ", name, " texture!");
				return nullptr;
			}
			return sampler;
		};
		auto createTexture = [&](size_t colorAttachmentId, const char* name) -> Reference<Graphics::TextureSampler> {
			Reference<Graphics::TextureSampler> sampler = createTextureView(ATTACHMENT_FORMATS[colorAttachmentId], name);
			if (sampler != nullptr)
				colorAttachments[colorAttachmentId] = sampler->TargetView();
			return sampler;
		};
		buffers.vertexPosition = createTexture(VERTEX_POSITION_ATTACHMENT_ID, "vertexPosition");
		buffers.vertexNormal = createTexture(VERTEX_NORMAL_ATTACHMENT_ID, "vertexNormal");
		buffers.objectIndex = createTexture(OBJECT_INDEX_ATTACHMENT_ID, "objectIndex");
		buffers.instanceIndex = createTexture(INSTANCE_INDEX_ATTACHMENT_ID, "instanceIndex");
		buffers.primitiveIndex = createTexture(PRIMITIVE_INDEX_ATTACHMENT_ID, "primitiveIndex");
		buffers.vertexNormalColor = createTexture(VERTEX_NORMAL_COLOR_ATTACHMENT_ID, "vertexNormalColor");
		buffers.depthAttachment = createTextureView(m_viewport->Context()->Graphics()->Device()->GetDepthFormat(), "depthAttachment");
		for (size_t i = 0; i < ColorAttachmentCount(); i++)
			if (colorAttachments[i] == nullptr) 
				return false;
		if (buffers.depthAttachment == nullptr)
			return false;

		// Create frame buffer:
		buffers.frameBuffer = dynamic_cast<PipelineObjects*>(m_pipelineObjects.operator->())->
			RenderPass()->CreateFrameBuffer(colorAttachments, buffers.depthAttachment->TargetView(), nullptr);
		if (buffers.frameBuffer == nullptr) {
			m_viewport->Context()->Log()->Error("ObjectIdRenderer::SetResolution - Failed to create frame buffer!");
			return false;
		}

		// Yep, if we got here, we succeeded...
		m_buffers = buffers;
		return true;
	}
}
