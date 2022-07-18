#include "ForwardLightingModel.h"
#include "../GraphicsEnvironment.h"
#include "../../SceneObjects/Lights/LightDataBuffer.h"
#include "../../SceneObjects/Lights/LightTypeIdBuffer.h"
#include "../../../../Graphics/Data/GraphicsPipelineSet.h"


namespace Jimara {
	namespace {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding> jimara_BindlessTextures =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessTextureSamplerSetBinding>("jimara_BindlessTextures");
			const Reference<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding> jimara_BindlessBuffers =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedBindlessStructuredBufferSetBinding>("jimara_BindlessBuffers");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_ForwardRenderer_LightTypeIds =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_ForwardRenderer_LightTypeIds");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ForwardRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_ForwardRenderer_ViewportBuffer");

		public:
			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_ForwardRenderer_ViewportBuffer->BindingName()) return jimara_ForwardRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else if (name == jimara_ForwardRenderer_LightTypeIds->BindingName()) return jimara_ForwardRenderer_LightTypeIds;
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
			const Reference<LightDataBuffer> m_lightDataBuffer;
			const Reference<LightTypeIdBuffer> m_lightTypeIdBuffer;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_lightDataBuffer(LightDataBuffer::Instance(viewport->Context()))
				, m_lightTypeIdBuffer(LightTypeIdBuffer::Instance(viewport->Context()))
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
				jimara_BindlessTextures->BoundObject() = m_viewport->Context()->Graphics()->Bindless().SamplerBinding();
				jimara_BindlessBuffers->BoundObject() = m_viewport->Context()->Graphics()->Bindless().BufferBinding();
				jimara_ForwardRenderer_ViewportBuffer->BoundObject() = m_viewportBuffer;
			}

			inline void Update(float aspect) {
				jimara_LightDataBinding->BoundObject() = m_lightDataBuffer->Buffer();
				jimara_ForwardRenderer_LightTypeIds->BoundObject() = m_lightTypeIdBuffer->Buffer();
				ViewportBuffer_t& buffer = m_viewportBuffer.Map();
				buffer.view = m_viewport->ViewMatrix();
				buffer.projection = m_viewport->ProjectionMatrix(aspect);
				m_viewportBuffer->Unmap(true);
			}

			inline void GetDependencies(Callback<JobSystem::Job*> report) {
				report(m_lightDataBuffer);
				report(m_lightTypeIdBuffer);
			}
		};





		/** GraphicsObjectDescriptor TO Graphics::GraphicsPipeline::Descriptor TRANSLATION PER GraphicsContext INSTANCE: */

		struct PipelineDescPerObject {
			Reference<GraphicsObjectDescriptor> object;
			mutable Reference<Graphics::GraphicsPipeline::Descriptor> descriptor;

			inline PipelineDescPerObject(GraphicsObjectDescriptor* obj = nullptr) : object(obj) {}
		};

		class ForwordPipelineObjects : public virtual ObjectCache<Reference<Object>>::StoredObject {
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

			class DescriptorCreateJob {
			private:
				ForwordPipelineObjects* const m_objects;
				const PipelineDescPerObject* const m_added;
				const size_t m_numAdded;

			public:
				inline DescriptorCreateJob(ForwordPipelineObjects* objects, const PipelineDescPerObject* added, size_t numAdded)
					: m_objects(objects), m_added(added), m_numAdded(numAdded) {}

				inline void Execute(ThreadBlock::ThreadInfo info, void*) {
					const PipelineDescPerObject* end = (m_added + m_numAdded);
					for (const PipelineDescPerObject* ptr = m_added + info.threadId; ptr < end; ptr += info.threadCount) {
						if (ptr->object->ShaderClass() == nullptr) continue;
						ptr->descriptor = m_objects->m_environment->CreateGraphicsPipelineDescriptor(ptr->object);
#ifndef NDEBUG
						if (ptr->descriptor == nullptr) m_objects->m_context->Log()->Error(
							"ForwordPipelineObjects::DescriptorCreateJob - Failed to create graphics pipeline descriptor!");
#endif
					}
				}
			};

			template<typename DescriptorReferenceType>
			inline void OnObjectsAddedLockless(const DescriptorReferenceType* objects, size_t count) {
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
					if (numAdded != count) m_context->Log()->Error("ForwordPipelineObjects::OnObjectsAddedLockless - (numAdded != count)!");
#endif
					static const size_t MAX_THREADS = max(std::thread::hardware_concurrency(), 1u);
					const size_t MIN_OBJECTS_PER_THREAD = 32;
					const size_t threads = min((numAdded + MIN_OBJECTS_PER_THREAD - 1) / MIN_OBJECTS_PER_THREAD, MAX_THREADS);
					DescriptorCreateJob job(this, added, numAdded);
					if (threads <= 1) {
						ThreadBlock::ThreadInfo info;
						info.threadCount = 1;
						info.threadId = 0;
						job.Execute(info, nullptr);
					}
					else m_descriptorCreationBlock.Execute(threads, nullptr, Callback(&DescriptorCreateJob::Execute, job));
					m_onDescriptorsAdded(added, numAdded);
					});
			}

			template<typename DescriptorReferenceType>
			inline void OnObjectsRemovedLockless(const DescriptorReferenceType* objects, size_t count) {
				if (count <= 0) return;
				m_activeObjects.Remove(objects, count, [&](const PipelineDescPerObject* removed, size_t numRemoved) {
#ifndef NDEBUG
					if (numRemoved != count) m_context->Log()->Error("ForwardRenderer::OnObjectsRemovedLockless - (numRemoved != count)!");
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
			inline ForwordPipelineObjects(SceneContext* context)
				: m_context(context)
				, m_shaderSet(context->Graphics()->Configuration().ShaderLoader()
					->LoadShaderSet("Jimara/Environment/Rendering/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm"))
				, m_graphicsObjects(GraphicsObjectDescriptor::Set::GetInstance(context)) {
				if (m_shaderSet == nullptr) m_context->Log()->Fatal("ForwordPipelineObjects - Could not retrieve shader set!");

				m_graphicsObjects->OnAdded() += Callback(&ForwordPipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() += Callback(&ForwordPipelineObjects::OnObjectsRemoved, this);

				{
					std::unique_lock<std::recursive_mutex> lock(m_dataLock);
					const Reference<GraphicsObjectDescriptor>* objects;
					size_t objectCount;
					std::vector<Reference<GraphicsObjectDescriptor>> allObjects;
					m_graphicsObjects->GetAll([&](GraphicsObjectDescriptor* descriptor) { allObjects.push_back(descriptor); });
					objects = allObjects.data();
					objectCount = allObjects.size();
					OnObjectsAddedLockless(objects, objectCount);
				}
			}

			inline virtual ~ForwordPipelineObjects() {
				m_graphicsObjects->OnAdded() -= Callback(&ForwordPipelineObjects::OnObjectsAdded, this);
				m_graphicsObjects->OnRemoved() -= Callback(&ForwordPipelineObjects::OnObjectsRemoved, this);
			}

			class Reader : public virtual std::unique_lock<std::recursive_mutex> {
			private:
				const Reference<const ForwordPipelineObjects> m_objects;

			public:
				inline Reader(const ForwordPipelineObjects* objects)
					: std::unique_lock<std::recursive_mutex>(objects->m_dataLock), m_objects(objects) {}

				inline Event<const PipelineDescPerObject*, size_t>& OnDescriptorsAdded()const { return m_objects->m_onDescriptorsAdded; }
				inline Event<const PipelineDescPerObject*, size_t>& OnDescriptorsRemoved()const { return m_objects->m_onDescriptorsRemoved; }
				inline void GetDescriptorData(const PipelineDescPerObject*& data, size_t& count) {
					data = m_objects->m_activeObjects.Data();
					count = m_objects->m_activeObjects.Size();
				}
				inline Graphics::ShaderSet* ShaderSet()const { return m_objects->m_shaderSet; }
			};
		};

		class ForwordPipelineObjectCache : public virtual ObjectCache<Reference<Object>> {
		public:
			inline static Reference<ForwordPipelineObjects> GetObjects(SceneContext* context) {
				static ForwordPipelineObjectCache cache;
				return cache.GetCachedOrCreate(context, false, [&]()->Reference<ForwordPipelineObjects> { return Object::Instantiate<ForwordPipelineObjects>(context); });
			}
		};





		/** FORWARD RENDERER: */

		class ForwardRenderer : public virtual RenderStack::Renderer {
		private:
			const Reference<const ViewportDescriptor> m_viewport;
			const LayerMask m_layerMask;
			const Graphics::RenderPass::Flags m_clearAndResolveFlags;
			const Reference<ForwordPipelineObjects> m_pipelineObjects;
			EnvironmentDescriptor m_environmentDescriptor;

			struct {
				Reference<Graphics::RenderPass> renderPass;
				Graphics::Texture::PixelFormat pixelFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::PixelFormat depthFormat = Graphics::Texture::PixelFormat::OTHER;
				Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::MAX_AVAILABLE;
			} m_renderPass;

			struct {
				Reference<Graphics::Pipeline> environmentPipeline;
				Reference<Graphics::GraphicsPipelineSet> pipelineSet;
			} m_pipelines;

			struct {
				Reference<RenderImages> renderImages;
				Reference<Graphics::FrameBuffer> frameBuffer;
			} m_lastFrameBuffer;

			template<typename ChangeCallback>
			inline void UpdateSet(const PipelineDescPerObject* objects, size_t count, const ChangeCallback& changeCallback) {
				static thread_local std::vector<Reference<Graphics::GraphicsPipeline::Descriptor>> descriptors;
				descriptors.resize(count);
				for (size_t i = 0; i < count; i++) {
					const PipelineDescPerObject& object = objects[i];
					if (object.object == nullptr) continue;
					else if (!m_layerMask[object.object->Layer()]) continue;
					else descriptors[i] = objects[i].descriptor;
				}
				changeCallback(descriptors.data(), descriptors.size());
				descriptors.clear();
			}

			inline void AddObjects(const PipelineDescPerObject* objects, size_t count) {
				if (m_pipelines.pipelineSet == nullptr) return;
				else if (m_pipelines.environmentPipeline == nullptr) {
					ForwordPipelineObjects::Reader reader(m_pipelineObjects);
					for (size_t i = 0; i < count; i++) {
						GraphicsObjectDescriptor* sampleObject = objects[i].object;
						if (sampleObject == nullptr) continue;
						Reference<GraphicsEnvironment> environment = GraphicsEnvironment::Create(
							reader.ShaderSet(), m_environmentDescriptor, sampleObject, m_viewport->Context()->Graphics()->Device());
						if (environment == nullptr) continue;
						Reference<Graphics::PipelineDescriptor> descriptor = environment->EnvironmentDescriptor();
						if (descriptor == nullptr) continue;
						m_pipelines.environmentPipeline = m_viewport->Context()->Graphics()->Device()->CreateEnvironmentPipeline(
							descriptor, m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount());
						if (m_pipelines.environmentPipeline != nullptr) break;
					}
				}
				UpdateSet(objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) { 
					m_pipelines.pipelineSet->AddPipelines(descs, numDescs);
					});
				
			}

			inline void RemoveObjects(const PipelineDescPerObject* objects, size_t count) {
				if (m_pipelines.pipelineSet != nullptr)
					UpdateSet(objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) {
					m_pipelines.pipelineSet->RemovePipelines(descs, numDescs);
						});
			}
			
			inline bool RefreshPipelines() {
				ForwordPipelineObjects::Reader reader(m_pipelineObjects);
				m_pipelines.environmentPipeline = nullptr;
				m_pipelines.pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(
					m_viewport->Context()->Graphics()->Device()->GraphicsQueue(), m_renderPass.renderPass, 
					m_viewport->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(),
					max(std::thread::hardware_concurrency() / 2, 1u));
				const PipelineDescPerObject* objects;
				size_t count;
				reader.GetDescriptorData(objects, count);
				AddObjects(objects, count);
				return true;
			}

			inline bool RefreshRenderPass(
				Graphics::Texture::PixelFormat pixelFormat, 
				Graphics::Texture::PixelFormat depthFormat,
				Graphics::Texture::Multisampling sampleCount) {
				
				if (m_renderPass.renderPass != nullptr && 
					m_renderPass.pixelFormat == pixelFormat && 
					m_renderPass.sampleCount == sampleCount) return true;
				
				m_renderPass.pixelFormat = pixelFormat;
				m_renderPass.depthFormat = depthFormat;
				m_renderPass.sampleCount = sampleCount;

				m_renderPass.renderPass = m_viewport->Context()->Graphics()->Device()->CreateRenderPass(
					m_renderPass.sampleCount, 1, &m_renderPass.pixelFormat, m_renderPass.depthFormat, m_clearAndResolveFlags);
				if (m_renderPass.renderPass == nullptr) {
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - Error: Failed to (re)create the render pass!");
					return false;
				}
				else return RefreshPipelines();
			}
			
			inline Reference<Graphics::FrameBuffer> RefreshFrameBuffer(RenderImages* images) {
				if (m_lastFrameBuffer.renderImages == images)
					return m_lastFrameBuffer.frameBuffer;

				RenderImages::Image* mainColor = images->GetImage(RenderImages::MainColor());
				RenderImages::Image* depthBuffer = images->GetImage(RenderImages::DepthBuffer());

				Reference<Graphics::TextureView> depthAttachment = depthBuffer->Multisampled();
				Reference<Graphics::TextureView> depthResolve = depthBuffer->Resolve();
				Reference<Graphics::TextureView> colorAttachment = mainColor->Multisampled();
				Reference<Graphics::TextureView> resolveAttachment = mainColor->Resolve();

				if (!RefreshRenderPass(
					colorAttachment->TargetTexture()->ImageFormat(),
					depthAttachment->TargetTexture()->ImageFormat(),
					images->SampleCount())) return nullptr;

				m_lastFrameBuffer.frameBuffer = m_renderPass.renderPass->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveAttachment, depthResolve);
				if (m_lastFrameBuffer.frameBuffer == nullptr)
					m_viewport->Context()->Log()->Error("ForwardRenderer::RefreshRenderPass - Failed to create the frame buffer!");
				else m_lastFrameBuffer.renderImages = images;

				return m_lastFrameBuffer.frameBuffer;
			}

		public:
			inline ForwardRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)
				: m_viewport(viewport)
				, m_layerMask(layers)
				, m_clearAndResolveFlags(flags)
				, m_pipelineObjects(ForwordPipelineObjectCache::GetObjects(viewport->Context()))
				, m_environmentDescriptor(viewport) {
				ForwordPipelineObjects::Reader reader(m_pipelineObjects);
				reader.OnDescriptorsAdded() += Callback(&ForwardRenderer::AddObjects, this);
				reader.OnDescriptorsRemoved() += Callback(&ForwardRenderer::RemoveObjects, this);
			}

			inline virtual ~ForwardRenderer() {
				ForwordPipelineObjects::Reader reader(m_pipelineObjects);
				reader.OnDescriptorsAdded() -= Callback(&ForwardRenderer::AddObjects, this);
				reader.OnDescriptorsRemoved() -= Callback(&ForwardRenderer::RemoveObjects, this);
			}

			inline virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, RenderImages* images) final override {
				if (images == nullptr) return;

				ForwordPipelineObjects::Reader readLock(m_pipelineObjects);
				Reference<Graphics::FrameBuffer> frameBuffer = RefreshFrameBuffer(images);
				if (frameBuffer == nullptr) return;

				Size2 size = images->Resolution();
				if (size.x <= 0 || size.y <= 0) return;
				m_environmentDescriptor.Update(((float)size.x) / ((float)size.y));

				Graphics::PrimaryCommandBuffer* buffer = dynamic_cast<Graphics::PrimaryCommandBuffer*>(commandBufferInfo.commandBuffer);
				if (buffer == nullptr) {
					m_viewport->Context()->Log()->Error("ForwardRenderer::Render - bufferInfo.commandBuffer should be a primary command buffer!");
					return;
				}

				const Vector4 clearColor = m_viewport->ClearColor();
				const Vector4* CLEAR_VALUE = &clearColor;
				m_renderPass.renderPass->BeginPass(buffer, frameBuffer, CLEAR_VALUE, true);
				if (m_pipelines.environmentPipeline != nullptr)
					m_pipelines.pipelineSet->ExecutePipelines(buffer, commandBufferInfo.inFlightBufferId, frameBuffer, m_pipelines.environmentPipeline);
				m_renderPass.renderPass->EndPass(buffer);
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				m_environmentDescriptor.GetDependencies(report);
			}
		};
	}

	Reference<ForwardLightingModel> ForwardLightingModel::Instance() {
		static ForwardLightingModel model;
		return &model;
	}

	Reference<RenderStack::Renderer> ForwardLightingModel::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags) {
		if (viewport == nullptr) return nullptr;
		else return Object::Instantiate<ForwardRenderer>(viewport, layers, flags);
	}
}
