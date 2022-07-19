#include "DepthOnlyRenderer.h"
#include "../GraphicsEnvironment.h"


namespace Jimara {
	struct DepthOnlyRenderer::Helpers {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding> jimara_BindlessTextures =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding>("jimara_BindlessTextures");
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding> jimara_BindlessBuffers =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding>("jimara_BindlessBuffers");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_DepthOnlyRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_DepthOnlyRenderer_ViewportBuffer");

		public:
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_DepthOnlyRenderer_ViewportBuffer->BindingName()) return jimara_DepthOnlyRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override {
				if (name == jimara_BindlessBuffers->BindingName()) return jimara_BindlessBuffers;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override {
				if (name == jimara_BindlessTextures->BindingName()) return jimara_BindlessTextures;
				else return nullptr;
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

			const Reference<const ViewportDescriptor> m_viewport;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
				jimara_BindlessTextures->BoundObject() = m_viewport->Context()->Graphics()->Bindless().SamplerBinding();
				jimara_BindlessBuffers->BoundObject() = m_viewport->Context()->Graphics()->Bindless().BufferBinding();
				jimara_LightDataBinding->BoundObject() = viewport->Context()->Graphics()->Device()->CreateArrayBuffer(
					m_viewport->Context()->Graphics()->Configuration().ShaderLoader()->PerLightDataSize(), 1);
				jimara_DepthOnlyRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
			}

			inline void Update(float aspect) {
				ViewportBuffer_t& buffer = m_viewportBuffer.Map();
				buffer.view = m_viewport->ViewMatrix();
				buffer.projection = m_viewport->ProjectionMatrix(aspect);
				m_viewportBuffer->Unmap(true);
			}
		};


		/** GraphicsObjectDescriptor TO Graphics::GraphicsPipeline::Descriptor TRANSLATION PER GraphicsContext INSTANCE: */

		struct PipelineDescPerObject {
			Reference<GraphicsObjectDescriptor> object;
			mutable Reference<Graphics::GraphicsPipeline::Descriptor> descriptor;

			inline PipelineDescPerObject(GraphicsObjectDescriptor* obj = nullptr) : object(obj) {}
		};

		class PipelineObjects : public virtual ObjectCache<Reference<Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<Graphics::ShaderSet> m_shaderSet;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjects;
			std::recursive_mutex mutable m_dataLock;
			Reference<GraphicsEnvironment> m_environment;
			ObjectSet<GraphicsObjectDescriptor, PipelineDescPerObject> m_activeObjects;
			mutable EventInstance<const PipelineDescPerObject*, size_t> m_onDescriptorsAdded;
			mutable EventInstance<const PipelineDescPerObject*, size_t> m_onDescriptorsRemoved;
			ThreadBlock m_descriptorCreationBlock;

			template<typename DescriptorReferenceType>
			inline void OnObjectsAddedLockless(const DescriptorReferenceType* objects, size_t count) {
				if (count <= 0) return;

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
					if (numAdded != count) 
						m_context->Log()->Error("DepthOnlyRenderer::PipelineObjects::OnObjectsAddedLockless - (numAdded != count)! ", 
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif
					static const size_t MAX_THREADS = max(std::thread::hardware_concurrency(), 1u);
					const size_t MIN_OBJECTS_PER_THREAD = 32;
					const size_t threads = min((numAdded + MIN_OBJECTS_PER_THREAD - 1) / MIN_OBJECTS_PER_THREAD, MAX_THREADS);
					
					auto createDescriptor = [&](ThreadBlock::ThreadInfo info, void*) {
						const PipelineDescPerObject* end = (added + numAdded);
						for (const PipelineDescPerObject* ptr = added + info.threadId; ptr < end; ptr += info.threadCount) {
							if (ptr->object->ShaderClass() == nullptr) continue;
							ptr->descriptor = m_environment->CreateGraphicsPipelineDescriptor(ptr->object);
#ifndef NDEBUG
							if (ptr->descriptor == nullptr) 
								m_context->Log()->Error("DepthOnlyRenderer::PipelineObjects::OnObjectsAddedLockless - Failed to create graphics pipeline descriptor! ", 
									"[File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif
						}
					};
					
					if (threads <= 1) {
						ThreadBlock::ThreadInfo info;
						info.threadCount = 1;
						info.threadId = 0;
						createDescriptor(info, nullptr);
					}
					else m_descriptorCreationBlock.Execute(threads, nullptr, Callback<ThreadBlock::ThreadInfo, void*>::FromCall(&createDescriptor));

					m_onDescriptorsAdded(added, numAdded);
					});
			}

			template<typename DescriptorReferenceType>
			inline void OnObjectsRemovedLockless(const DescriptorReferenceType* objects, size_t count) {
				if (count <= 0) return;
				m_activeObjects.Remove(objects, count, [&](const PipelineDescPerObject* removed, size_t numRemoved) {
#ifndef NDEBUG
					if (numRemoved != count) 
						m_context->Log()->Error("DepthOnlyRenderer::PipelineObjects::OnObjectsRemovedLockless - (numRemoved != count)! ",
							"[File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif
					m_onDescriptorsRemoved(removed, numRemoved);
					});
			}

			typedef GraphicsObjectDescriptor* ObjectAddedOrRemovedReference_t;

			inline void OnObjectsAdded(const ObjectAddedOrRemovedReference_t* objects, size_t count) {
				std::unique_lock<std::recursive_mutex> lock(m_dataLock);
				OnObjectsAddedLockless(objects, count);
			}

			inline void OnObjectsRemoved(const ObjectAddedOrRemovedReference_t* objects, size_t count) {
				std::unique_lock<std::recursive_mutex> lock(m_dataLock);
				OnObjectsRemovedLockless(objects, count);
			}

		public:
			inline PipelineObjects(SceneContext* context) 
				: m_context(context)
				, m_shaderSet(context->Graphics()->Configuration().ShaderLoader()
					->LoadShaderSet("Jimara/Environment/Rendering/LightingModels/DepthOnlyRenderer/Jimara_DepthOnlyRenderer.jlm"))
				, m_graphicsObjects(GraphicsObjectDescriptor::Set::GetInstance(context)) {
				if (m_shaderSet == nullptr) 
					m_context->Log()->Fatal("DepthOnlyRenderer::PipelineObjects - Could not retrieve shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				m_graphicsObjects->OnAdded() += Callback(&PipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() += Callback(&PipelineObjects::OnObjectsRemoved, this);

				{
					std::unique_lock<std::recursive_mutex> lock(m_dataLock);
					std::vector<Reference<GraphicsObjectDescriptor>> allObjects;
					m_graphicsObjects->GetAll([&](GraphicsObjectDescriptor* descriptor) { allObjects.push_back(descriptor); });
					OnObjectsAddedLockless(allObjects.data(), allObjects.size());
				}
			}

			inline virtual ~PipelineObjects() {
				m_graphicsObjects->OnAdded() -= Callback(&PipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() -= Callback(&PipelineObjects::OnObjectsRemoved, this);
			}

			class Reader : public virtual std::unique_lock<std::recursive_mutex> {
			private:
				const Reference<const PipelineObjects> m_objects;

			public:
				inline Reader(const Object* objects)
					: std::unique_lock<std::recursive_mutex>(dynamic_cast<const PipelineObjects*>(objects)->m_dataLock)
					, m_objects(dynamic_cast<const PipelineObjects*>(objects)) {}

				inline Event<const PipelineDescPerObject*, size_t>& OnDescriptorsAdded()const { return m_objects->m_onDescriptorsAdded; }
				inline Event<const PipelineDescPerObject*, size_t>& OnDescriptorsRemoved()const { return m_objects->m_onDescriptorsRemoved; }
				inline void GetDescriptorData(const PipelineDescPerObject*& data, size_t& count) {
					data = m_objects->m_activeObjects.Data();
					count = m_objects->m_activeObjects.Size();
				}
				inline Graphics::ShaderSet* ShaderSet()const { return m_objects->m_shaderSet; }
			};
		};

		class PipelineObjectCache : public virtual ObjectCache<Reference<Object>> {
		public:
			inline static Reference<PipelineObjects> GetObjects(SceneContext* context) {
				static PipelineObjectCache cache;
				return cache.GetCachedOrCreate(context, false, [&]()->Reference<PipelineObjects> { return Object::Instantiate<PipelineObjects>(context); });
			}
		};

		template<typename ChangeCallback>
		inline static void UpdateSet(DepthOnlyRenderer* self, const PipelineDescPerObject* objects, size_t count, const ChangeCallback& changeCallback) {
			static thread_local std::vector<Reference<Graphics::GraphicsPipeline::Descriptor>> descriptors;
			descriptors.resize(count);
			for (size_t i = 0; i < count; i++) {
				const PipelineDescPerObject& object = objects[i];
				if (object.object == nullptr) continue;
				else if (!self->m_layerMask[object.object->Layer()]) continue;
				else descriptors[i] = objects[i].descriptor;
			}
			changeCallback(descriptors.data(), descriptors.size());
			descriptors.clear();
		}

		inline static void AddObjects(DepthOnlyRenderer* self, const PipelineDescPerObject* objects, size_t count) {
			if (self->m_pipelineSet == nullptr) return;
			else if (self->m_environmentPipeline == nullptr) {
				PipelineObjects::Reader reader(self->m_pipelineObjects);
				for (size_t i = 0; i < count; i++) {
					GraphicsObjectDescriptor* sampleObject = objects[i].object;
					if (sampleObject == nullptr) continue;
					Reference<GraphicsEnvironment> environment = GraphicsEnvironment::Create(
						reader.ShaderSet(), *self->m_environmentDescriptor, sampleObject, self->m_viewport->Context()->Graphics()->Device());
					if (environment == nullptr) continue;
					Reference<Graphics::PipelineDescriptor> descriptor = environment->EnvironmentDescriptor();
					if (descriptor == nullptr) continue;
					self->m_environmentPipeline = self->m_viewport->Context()->Graphics()->Device()->CreateEnvironmentPipeline(
						descriptor, self->m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
					if (self->m_environmentPipeline != nullptr) break;
				}
			}
			UpdateSet(self, objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) {
				self->m_pipelineSet->AddPipelines(descs, numDescs);
				});
		}
		inline static void RemoveObjects(DepthOnlyRenderer* self, const PipelineDescPerObject* objects, size_t count) {
			if (self->m_pipelineSet != nullptr)
				UpdateSet(self, objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) {
				self->m_pipelineSet->RemovePipelines(descs, numDescs);
					});
		}





		inline static bool RecreatePipelines(DepthOnlyRenderer* self) {
			PipelineObjects::Reader reader(self->m_pipelineObjects);
			self->m_environmentPipeline = nullptr;
			self->m_pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(
				self->m_viewport->Context()->Graphics()->Device()->GraphicsQueue(), self->m_renderPass,
				self->m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(),
				1u); // We'll mostly be using this for shadow rendering, so there will be a lot of instances... Multithreaded pipeline sets are a lame idea here...
			const PipelineDescPerObject* objects;
			size_t count;
			reader.GetDescriptorData(objects, count);
			AddObjects(self, objects, count);
			return true;
		}

		inline static bool RefreshRenderPass(DepthOnlyRenderer* self, Graphics::Texture::PixelFormat depthFormat, Graphics::Texture::Multisampling sampleCount) {
			if (self->m_renderPass != nullptr && 
				self->m_renderPass->DepthAttachmentFormat() == depthFormat && 
				self->m_renderPass->SampleCount() == sampleCount) return true;

			self->m_renderPass = self->m_viewport->Context()->Graphics()->Device()->CreateRenderPass(
				sampleCount, 0u, nullptr, depthFormat, Graphics::RenderPass::Flags::CLEAR_DEPTH);
			if (self->m_renderPass == nullptr) {
				self->m_viewport->Context()->Log()->Error("DepthOnlyRenderer::Helpers::RefreshRenderPass - Error: Failed to (re)create the render pass! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			else return RecreatePipelines(self);
		}

		inline static Reference<Graphics::FrameBuffer> RefreshFrameBuffer(DepthOnlyRenderer* self) {
			const Reference<Graphics::TextureView> targetTexture = [&]() {
				std::unique_lock<SpinLock> bufferLock(self->m_textureLock);
				Reference<Graphics::TextureView> rv = self->m_targetTexture;
				return rv;
			}();

			if (targetTexture == self->m_frameBufferTexture)
				return self->m_frameBuffer;

			self->m_frameBuffer = nullptr;
			self->m_frameBufferTexture = nullptr;

			if (targetTexture == nullptr) 
				return nullptr;

			if (!RefreshRenderPass(self, targetTexture->TargetTexture()->ImageFormat(), targetTexture->TargetTexture()->SampleCount()))
				return nullptr;

			self->m_frameBuffer = self->m_renderPass->CreateFrameBuffer(nullptr, targetTexture, nullptr, nullptr);
			if (self->m_frameBuffer == nullptr)
				self->m_viewport->Context()->Log()->Error("DepthOnlyRenderer::Helpers::RefreshRenderPass - Failed to create the frame buffer! ",
					"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			else self->m_frameBufferTexture = targetTexture;
			
			return self->m_frameBuffer;
		}
	};

	DepthOnlyRenderer::DepthOnlyRenderer(const ViewportDescriptor* viewport, LayerMask layers) 
		: m_viewport(viewport), m_layerMask(layers)
		, m_pipelineObjects(Helpers::PipelineObjectCache::GetObjects(viewport->Context()))
		, m_environmentDescriptor(Object::Instantiate<Helpers::EnvironmentDescriptor>(viewport)) {
		Helpers::PipelineObjects::Reader reader(m_pipelineObjects);
		reader.OnDescriptorsAdded() += Callback(Helpers::AddObjects, this);
		reader.OnDescriptorsRemoved() += Callback(Helpers::RemoveObjects, this);
	}

	DepthOnlyRenderer::~DepthOnlyRenderer() {
		Helpers::PipelineObjects::Reader reader(m_pipelineObjects);
		reader.OnDescriptorsAdded() -= Callback(Helpers::AddObjects, this);
		reader.OnDescriptorsRemoved() -= Callback(Helpers::RemoveObjects, this);
	}

	Graphics::Texture::PixelFormat DepthOnlyRenderer::TargetTextureFormat()const {
		return m_viewport->Context()->Graphics()->Device()->GetDepthFormat();
	}

	void DepthOnlyRenderer::SetTargetTexture(Graphics::TextureView* depthTexture) {
		std::unique_lock<SpinLock> bufferLock(m_textureLock);
		if (depthTexture == m_targetTexture) return;
		else if (depthTexture->TargetTexture()->ImageFormat() != TargetTextureFormat()) {
			m_viewport->Context()->Log()->Error("DepthOnlyRenderer::SetTargetTexture - Texture format (", (uint32_t)depthTexture->TargetTexture()->ImageFormat(), ") not supported!");
		}
		else m_targetTexture = depthTexture;
	}

	void DepthOnlyRenderer::Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo) {
		Helpers::PipelineObjects::Reader reader(m_pipelineObjects);

		const Reference<Graphics::FrameBuffer> frameBuffer = Helpers::RefreshFrameBuffer(this);
		if (frameBuffer == nullptr) return;

		const Size2 resolution = frameBuffer->Resolution();
		if (resolution.x <= 0 || resolution.y <= 0) return;
		dynamic_cast<Helpers::EnvironmentDescriptor*>(m_environmentDescriptor.operator->())->Update(((float)resolution.x) / ((float)resolution.y));

		Graphics::PrimaryCommandBuffer* buffer = dynamic_cast<Graphics::PrimaryCommandBuffer*>(commandBufferInfo.commandBuffer);
		if (buffer == nullptr) {
			m_viewport->Context()->Log()->Error("DepthOnlyRenderer::Render - bufferInfo.commandBuffer should be a primary command buffer!");
			return;
		}

		const Vector4 clearColor(0.0f);
		m_renderPass->BeginPass(buffer, frameBuffer, &clearColor, true);
		if (m_environmentPipeline != nullptr)
			m_pipelineSet->ExecutePipelines(buffer, commandBufferInfo.inFlightBufferId, frameBuffer, m_environmentPipeline);
		m_renderPass->EndPass(buffer);
	}

	void DepthOnlyRenderer::Execute() { Render(m_viewport->Context()->Graphics()->GetWorkerThreadCommandBuffer()); }

	void DepthOnlyRenderer::CollectDependencies(Callback<JobSystem::Job*>) {}
}
