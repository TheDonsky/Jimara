#include "ForwardLightingModel.h"
#include "../GraphicsEnvironment.h"
#include "../../Lights/LightDataBuffer.h"
#include "../../Lights/LightTypeIdBuffer.h"
#include "../../../../Graphics/Data/GraphicsPipelineSet.h"


namespace Jimara {
	namespace {
		/** ENVIRONMENT SHAPE DESCRIPTOR: */
		class EnvironmentShapeDescriptor : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		protected:
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_LightDataBinding =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_LightDataBinding");
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> jimara_ForwardRenderer_LightTypeIds =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("jimara_ForwardRenderer_LightTypeIds");
			const Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding> jimara_ForwardRenderer_ViewportBuffer =
				Object::Instantiate<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>("jimara_ForwardRenderer_ViewportBuffer");

		public:
			inline virtual const Graphics::ShaderResourceBindings::ConstantBufferBinding* FindConstantBufferBinding(const std::string& name)const override {
				if (name == jimara_ForwardRenderer_ViewportBuffer->BindingName()) return jimara_ForwardRenderer_ViewportBuffer;
				else return nullptr;
			}

			inline virtual const Graphics::ShaderResourceBindings::StructuredBufferBinding* FindStructuredBufferBinding(const std::string& name)const override {
				if (name == jimara_LightDataBinding->BindingName()) return jimara_LightDataBinding;
				else if (name == jimara_ForwardRenderer_LightTypeIds->BindingName()) return jimara_ForwardRenderer_LightTypeIds;
				else return nullptr;
			}

			inline virtual const Graphics::ShaderResourceBindings::TextureSamplerBinding* FindTextureSamplerBinding(const std::string&)const override {
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
			const Reference<const LightDataBuffer> m_lightDataBuffer;
			const Reference<const LightTypeIdBuffer> m_lightTypeIdBuffer;
			const Graphics::BufferReference<ViewportBuffer_t> m_viewportBuffer;

		public:
			inline EnvironmentDescriptor(const LightingModel::ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_lightDataBuffer(LightDataBuffer::Instance(viewport->Context()))
				, m_lightTypeIdBuffer(LightTypeIdBuffer::Instance(viewport->Context()))
				, m_viewportBuffer(viewport->Context()->Graphics()->Device()->CreateConstantBuffer<ViewportBuffer_t>()) {
				if (m_viewportBuffer == nullptr) m_viewport->Context()->Log()->Fatal("ForwardLightingModel - Could not create Viewport Buffer!");
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
					->LoadShaderSet("Jimara/Environment/GraphicsContext/LightingModels/ForwardRendering/Jimara_ForwardRenderer.jlm"))
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





		/** FORWARD RENDERER DATA: */

		class ForwardRendererData : public virtual Object {
		private:
			const Reference<Graphics::RenderEngineInfo> m_engineInfo;
			const Reference<ForwordPipelineObjects> m_pipelineObjects;
			const Reference<const LightingModel::ViewportDescriptor> m_viewport;

			Reference<Graphics::RenderPass> m_renderPass;
			std::vector<Reference<Graphics::FrameBuffer>> m_frameBuffers;
			
			EnvironmentDescriptor m_environmentDescriptor;
			Reference<Graphics::Pipeline> m_environmentPipeline;
			Reference<Graphics::GraphicsPipelineSet> m_pipelineSet;

			template<typename ChangeCallback>
			inline void UpdateSet(const PipelineDescPerObject* objects, size_t count, const ChangeCallback& changeCallback) {
				static thread_local std::vector<Reference<Graphics::GraphicsPipeline::Descriptor>> descriptors;
				descriptors.resize(count);
				for (size_t i = 0; i < count; i++) descriptors[i] = objects[i].descriptor;
				changeCallback(descriptors.data(), descriptors.size());
				descriptors.clear();
			}

			inline void AddObjects(const PipelineDescPerObject* objects, size_t count) {
				if (m_environmentPipeline == nullptr) {
					ForwordPipelineObjects::Reader reader(m_pipelineObjects);
					for (size_t i = 0; i < count; i++) {
						GraphicsObjectDescriptor* sampleObject = objects[i].object;
						if (sampleObject == nullptr) continue;
						Reference<GraphicsEnvironment> environment = GraphicsEnvironment::Create(reader.ShaderSet(), m_environmentDescriptor, sampleObject, m_engineInfo->Device());
						if (environment == nullptr) continue;
						Reference<Graphics::PipelineDescriptor> descriptor = environment->EnvironmentDescriptor();
						if (descriptor == nullptr) continue;
						m_environmentPipeline = m_engineInfo->Device()->CreateEnvironmentPipeline(descriptor, m_engineInfo->ImageCount());
						if (m_environmentPipeline != nullptr) break;
					}
				}
				UpdateSet(objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) { m_pipelineSet->AddPipelines(descs, numDescs); });
				
			}

			inline void RemoveObjects(const PipelineDescPerObject* objects, size_t count) {
				UpdateSet(objects, count, [&](const Reference<Graphics::GraphicsPipeline::Descriptor>* descs, size_t numDescs) { m_pipelineSet->RemovePipelines(descs, numDescs); });
			}

		public:
			inline ForwardRendererData(Graphics::RenderEngineInfo* engineInfo, ForwordPipelineObjects* pipelineObjects, const LightingModel::ViewportDescriptor* viewport)
				: m_engineInfo(engineInfo)
				, m_pipelineObjects(pipelineObjects)
				, m_viewport(viewport)
				, m_environmentDescriptor(viewport) {

				const Graphics::Texture::PixelFormat PIXEL_FORMAT = m_engineInfo->ImageFormat();
				const Graphics::Texture::Multisampling SAMPLE_COUNT =
					/*
					Graphics::Texture::Multisampling::SAMPLE_COUNT_1
					/*/
					engineInfo->Device()->PhysicalDevice()->MaxMultisapling()
					/**/;
				const bool USE_CLEAR_COLOR = true;

				Reference<Graphics::TextureView> depthAttachment = m_engineInfo->Device()->CreateMultisampledTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, m_engineInfo->Device()->GetDepthFormat()
					, Size3(m_engineInfo->ImageSize(), 1), 1, SAMPLE_COUNT)
					->CreateView(Graphics::TextureView::ViewType::VIEW_2D);

				if (SAMPLE_COUNT != Graphics::Texture::Multisampling::SAMPLE_COUNT_1) {
					Reference<Graphics::TextureView> colorAttachment = m_engineInfo->Device()->CreateMultisampledTexture(
						Graphics::Texture::TextureType::TEXTURE_2D, PIXEL_FORMAT, depthAttachment->TargetTexture()->Size(), 1, SAMPLE_COUNT)
						->CreateView(Graphics::TextureView::ViewType::VIEW_2D);

					m_renderPass = m_engineInfo->Device()->CreateRenderPass(
						colorAttachment->TargetTexture()->SampleCount(), 1, &PIXEL_FORMAT, depthAttachment->TargetTexture()->ImageFormat(), true, USE_CLEAR_COLOR);

					for (size_t i = 0; i < m_engineInfo->ImageCount(); i++) {
						Reference<Graphics::TextureView> resolveView = engineInfo->Image(i)->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, depthAttachment, &resolveView));
					}
				}
				else {
					m_renderPass = m_engineInfo->Device()->CreateRenderPass(
						Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 1, &PIXEL_FORMAT, depthAttachment->TargetTexture()->ImageFormat(), false, USE_CLEAR_COLOR);

					for (size_t i = 0; i < m_engineInfo->ImageCount(); i++) {
						Reference<Graphics::TextureView> colorAttachment = engineInfo->Image(i)->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						m_frameBuffers.push_back(m_renderPass->CreateFrameBuffer(&colorAttachment, depthAttachment, nullptr));
					}
				}

				m_pipelineSet = Object::Instantiate<Graphics::GraphicsPipelineSet>(
					m_engineInfo->Device()->GraphicsQueue(), m_renderPass, m_engineInfo->ImageCount(),
					max(std::thread::hardware_concurrency() / 2, 1u));

				ForwordPipelineObjects::Reader reader(m_pipelineObjects);
				reader.OnDescriptorsAdded() += Callback(&ForwardRendererData::AddObjects, this);
				reader.OnDescriptorsRemoved() += Callback(&ForwardRendererData::RemoveObjects, this);
				const PipelineDescPerObject* objects;
				size_t count;
				reader.GetDescriptorData(objects, count);
				AddObjects(objects, count);
			}

			inline virtual ~ForwardRendererData() {
				ForwordPipelineObjects::Reader reader(m_pipelineObjects);
				reader.OnDescriptorsAdded() -= Callback(&ForwardRendererData::AddObjects, this);
				reader.OnDescriptorsRemoved() -= Callback(&ForwardRendererData::RemoveObjects, this);
			}

			inline void Render(const Graphics::Pipeline::CommandBufferInfo& bufferInfo) {
				ForwordPipelineObjects::Reader readLock(m_pipelineObjects);
				
				Size2 size = m_engineInfo->ImageSize();
				if (size.x <= 0 || size.y <= 0) return;
				m_environmentDescriptor.Update(((float)size.x) / ((float)size.y));

				Graphics::FrameBuffer* const frameBuffer = m_frameBuffers[bufferInfo.inFlightBufferId];
				const Vector4 CLEAR_VALUE = m_viewport->ClearColor();

				Graphics::PrimaryCommandBuffer* buffer = dynamic_cast<Graphics::PrimaryCommandBuffer*>(bufferInfo.commandBuffer);
				if (buffer == nullptr) {
					m_engineInfo->Device()->Log()->Error("ForwardRendererData::Render - bufferInfo.commandBuffer should be a primary command buffer!");
					return;
				}
				
				m_renderPass->BeginPass(buffer, frameBuffer, &CLEAR_VALUE, true);
				if (m_environmentPipeline != nullptr)
					m_pipelineSet->ExecutePipelines(buffer, bufferInfo.inFlightBufferId, frameBuffer, m_environmentPipeline);
				m_renderPass->EndPass(buffer);
			}
		};


		/** FORWARD RENDERER: */

		class ForwardRenderer : public virtual Graphics::ImageRenderer {
		private:
			const Reference<const LightingModel::ViewportDescriptor> m_viewport;
			const Reference<ForwordPipelineObjects> m_pipelineObjects;

		public:
			inline ForwardRenderer(const LightingModel::ViewportDescriptor* viewport)
				: m_viewport(viewport)
				, m_pipelineObjects(ForwordPipelineObjectCache::GetObjects(viewport->Context())) {}

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) {
				return Object::Instantiate<ForwardRendererData>(engineInfo, m_pipelineObjects, m_viewport);
			}

			inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) {
				dynamic_cast<ForwardRendererData*>(engineData)->Render(bufferInfo);
			}
		};
	}

	Reference<ForwardLightingModel> ForwardLightingModel::Instance() {
		static ForwardLightingModel model;
		return &model;
	}

	Reference<Graphics::ImageRenderer> ForwardLightingModel::CreateRenderer(const ViewportDescriptor* viewport) {
		return Object::Instantiate<ForwardRenderer>(viewport);
	}
}
