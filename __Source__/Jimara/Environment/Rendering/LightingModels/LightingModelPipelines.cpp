#include "LightingModelPipelines.h"
#include "../../../Math/Helpers.h"
#include "../../../Data/Materials/SampleDiffuse/SampleDiffuseShader.h"

namespace Jimara {
	struct LightingModelPipelines::Helpers {
		// Environment shape definition:
		struct BindingSetDescriptor : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			typedef Stacktor<BindingInfo, 4> BindingInfos;
			BindingInfos constantBuffers;
			BindingInfos structuredBuffers;
			BindingInfos textureSamplers;
			BindingInfos textureViews;
			enum class Type {
				NORMAL,
				BINDLESS_ARRAY_BUFFER_SET,
				BINDLESS_TEXTURE_SAMPLER_SET
			};
			Type type = Type::NORMAL;

			inline virtual bool SetByEnvironment()const override { return true; }
			
			inline virtual size_t ConstantBufferCount()const override { return constantBuffers.Size(); }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return constantBuffers[index]; }
			inline Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return nullptr; }

			inline virtual size_t StructuredBufferCount()const override { return structuredBuffers.Size(); }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return structuredBuffers[index]; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }

			inline virtual size_t TextureSamplerCount()const override { return textureSamplers.Size(); }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return textureSamplers[index]; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return nullptr; }

			inline virtual size_t TextureViewCount()const override { return textureViews.Size(); }
			inline virtual BindingInfo TextureViewInfo(size_t index)const override { return textureViews[index]; }
			inline virtual Reference<Graphics::TextureView> View(size_t)const override { return nullptr; }

			inline virtual bool IsBindlessArrayBufferArray()const override { return type == Type::BINDLESS_ARRAY_BUFFER_SET; }
			inline virtual Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> BindlessArrayBuffers()const override { return nullptr; }

			inline virtual bool IsBindlessTextureSamplerArray()const override { return type == Type::BINDLESS_TEXTURE_SAMPLER_SET; }
			inline virtual Reference<Graphics::BindlessSet<Graphics::TextureSampler>::Instance> BindlessTextureSamplers()const override { return nullptr; }
		};

		struct PipelineDescriptor : public virtual Graphics::PipelineDescriptor {
			Stacktor<Reference<Graphics::PipelineDescriptor::BindingSetDescriptor>> bindingSets;

			inline virtual size_t BindingSetCount()const override { return bindingSets.Size(); }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return bindingSets[index]; }
		};

		struct EnvironmentPipelineDescriptor : public virtual PipelineDescriptor {
			Reference<Graphics::SPIRV_Binary> vertexShader;
			Reference<Graphics::SPIRV_Binary> fragmentShader;
		};

#pragma warning(disable: 4250)
		struct GraphicsPipelineDescriptor 
			: public virtual PipelineDescriptor
			, public virtual Graphics::GraphicsPipeline::Descriptor
			, ObjectCache<Reference<const Object>>::StoredObject {
			
			Reference<GraphicsObjectDescriptor> graphicsObject;
			Reference<Graphics::Shader> vertexShader;
			Reference<Graphics::Shader> fragmentShader;

			inline virtual Reference<Graphics::Shader> VertexShader()const override { return vertexShader; }
			inline virtual Reference<Graphics::Shader> FragmentShader()const override { return fragmentShader; }

			inline virtual size_t VertexBufferCount()const override { return graphicsObject->VertexBufferCount(); }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override { return graphicsObject->VertexBuffer(index); }

			inline virtual size_t InstanceBufferCount()const override { return graphicsObject->InstanceBufferCount(); }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override { return graphicsObject->InstanceBuffer(index); }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override { return graphicsObject->IndexBuffer(); }

			inline virtual Graphics::GraphicsPipeline::IndexType GeometryType() override { return graphicsObject->GeometryType(); }
			inline virtual size_t IndexCount() override { return graphicsObject->IndexCount(); }
			inline virtual size_t InstanceCount() override { return graphicsObject->InstanceCount(); }
		};
#pragma warning(default: 4250)

		class SceneObjectResourceBindings : public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
		private:
			const GraphicsObjectDescriptor* m_sceneObject;
			const Graphics::ShaderClass* m_shaderClass;
			Graphics::GraphicsDevice* m_device;

		public:
			inline SceneObjectResourceBindings(const GraphicsObjectDescriptor* object, const Graphics::ShaderClass* shader, Graphics::GraphicsDevice* device)
				: m_sceneObject(object), m_shaderClass(shader), m_device(device) {}

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				const Graphics::ShaderResourceBindings::ConstantBufferBinding* objectBinding = m_sceneObject->FindConstantBufferBinding(name);
				if (objectBinding != nullptr) return objectBinding;
				else return m_shaderClass->DefaultConstantBufferBinding(name, m_device);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				const Graphics::ShaderResourceBindings::StructuredBufferBinding* objectBinding = m_sceneObject->FindStructuredBufferBinding(name);
				if (objectBinding != nullptr) return objectBinding;
				else return m_shaderClass->DefaultStructuredBufferBinding(name, m_device);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
				const Graphics::ShaderResourceBindings::TextureSamplerBinding* objectBinding = m_sceneObject->FindTextureSamplerBinding(name);
				if (objectBinding != nullptr) return objectBinding;
				else return m_shaderClass->DefaultTextureSamplerBinding(name, m_device);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string& name)const override {
				return m_sceneObject->FindTextureViewBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string& name)const override {
				return m_sceneObject->FindBindlessStructuredBufferSetBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string& name)const override {
				return m_sceneObject->FindBindlessTextureSamplerSetBinding(name);
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string& name)const override {
				return m_sceneObject->FindBindlessTextureSamplerSetBinding(name);
			}
		};

		inline static void GenerateEnvironmentPipeline(const Descriptor& modelDescriptor, Graphics::ShaderSet* shaderSet, EnvironmentPipelineDescriptor* environmentDescriptor) {
			static const Graphics::ShaderClass blankShader("Jimara/Environment/Rendering/LightingModels/Jimara_LightingModel_BlankShader");
			// Binding state:
			struct BindingState {
				BindingSetDescriptor* descriptorSet = nullptr;
				OS::Logger* log = nullptr;
				Graphics::PipelineStage stage = Graphics::PipelineStage::NONE;
				const OS::Path* lightingModelPath = nullptr;
				typedef size_t BindingIndex;
				typedef size_t BindingSlot;
				typedef std::unordered_map<BindingSlot, BindingIndex> BindingMappings;
				BindingMappings constantBuffers;
				BindingMappings structuredBuffers;
				BindingMappings textureSamplers;
				BindingMappings textureViews;
			};

			// Functions for updating binding state per binding type:
			typedef void(*IncludeBindingFn)(BindingState*, const Graphics::SPIRV_Binary::BindingInfo*);
			static const IncludeBindingFn* includeBinding = []() -> const IncludeBindingFn* {
				static const constexpr size_t BINDING_TYPE_COUNT = static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT);
				static IncludeBindingFn includeFunctions[BINDING_TYPE_COUNT];
				for (size_t i = 0; i < BINDING_TYPE_COUNT; i++) includeFunctions[i] = [](BindingState*, const Graphics::SPIRV_Binary::BindingInfo*) {};
				typedef void (*IncludeNormalBindingFn)(BindingSetDescriptor::BindingInfos&, BindingState::BindingMappings&, size_t, Graphics::PipelineStage);
				static const IncludeNormalBindingFn includeNormalBinding = [](BindingSetDescriptor::BindingInfos& infos, BindingState::BindingMappings& mappings, size_t bindingSlot, Graphics::PipelineStage stage) {
					const auto it = mappings.find(bindingSlot);
					if (it == mappings.end()) {
						mappings[bindingSlot] = infos.Size();
						Graphics::PipelineDescriptor::BindingSetDescriptor::BindingInfo info = {};
						{
							info.binding = static_cast<uint32_t>(bindingSlot);
							info.stages = Graphics::StageMask(stage);
						}
						infos.Push(info);
					}
					else infos[it->second].stages |= Graphics::StageMask(stage);
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					includeNormalBinding(state->descriptorSet->constantBuffers, state->constantBuffers, info->binding, state->stage);
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					includeNormalBinding(state->descriptorSet->textureSamplers, state->textureSamplers, info->binding, state->stage);
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					includeNormalBinding(state->descriptorSet->textureViews, state->textureViews, info->binding, state->stage);
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					includeNormalBinding(state->descriptorSet->structuredBuffers, state->structuredBuffers, info->binding, state->stage);
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER_ARRAY)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					state->log->Error("LightingModelPipelines - Bindless constant buffer arrays not yet supported [Set: ", info->set, "; Binding: ", info->binding, "] <",
						*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					if (info->binding == 0u) {
						if (state->descriptorSet->type == BindingSetDescriptor::Type::BINDLESS_TEXTURE_SAMPLER_SET || state->descriptorSet->type == BindingSetDescriptor::Type::NORMAL)
							state->descriptorSet->type = BindingSetDescriptor::Type::BINDLESS_TEXTURE_SAMPLER_SET;
						else state->log->Error("LightingModelPipelines - Same binding set can only contain a single bindless array [Set: ", info->set, "; Binding: ", info->binding, "] <",
							*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
					else state->log->Error("LightingModelPipelines - Bindless descriptor can only be bound to slot 0 [Set: ", info->set, "; Binding: ", info->binding, "] <",
						*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE_ARRAY)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					state->log->Error("LightingModelPipelines - Bindless storage image arrays not yet supported [Set: ", info->set, "; Binding: ", info->binding, "] <",
						*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};
				includeFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] = [](BindingState* state, const Graphics::SPIRV_Binary::BindingInfo* info) {
					if (info->binding == 0u) {
						if (state->descriptorSet->type == BindingSetDescriptor::Type::BINDLESS_ARRAY_BUFFER_SET || state->descriptorSet->type == BindingSetDescriptor::Type::NORMAL)
							state->descriptorSet->type = BindingSetDescriptor::Type::BINDLESS_ARRAY_BUFFER_SET;
						else state->log->Error("LightingModelPipelines - Same binding set can only contain a single bindless array [Set: ", info->set, "; Binding: ", info->binding, "] <",
							*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					}
					else state->log->Error("LightingModelPipelines - Bindless descriptor can only be bound to slot 0 [Set: ", info->set, "; Binding: ", info->binding, "] <",
						*state->lightingModelPath, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};
				return includeFunctions;
			}();

			// We store binding states per binding index:
			Stacktor<BindingState, 4> bindingStates;
			auto includeBinaryEntries = [&](Graphics::PipelineStage stage, Reference<Graphics::SPIRV_Binary>& binary) {
				if (binary == nullptr)
					binary = shaderSet->GetShaderModule(&blankShader, stage);
				if (binary == nullptr) {
					modelDescriptor.context->Log()->Error(
						"LightingModelPipelines - Failed to load blank shader module for stage ",
						static_cast<std::underlying_type_t<Graphics::PipelineStage>>(stage), " for lighting model: '", modelDescriptor.lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				for (size_t bindingSetId = 0; bindingSetId < binary->BindingSetCount(); bindingSetId++) {
					const Graphics::SPIRV_Binary::BindingSetInfo& bindingSet = binary->BindingSet(bindingSetId);
					while (environmentDescriptor->bindingSets.Size() <= bindingSetId) {
						Reference<BindingSetDescriptor> setDescriptor = Object::Instantiate<BindingSetDescriptor>();
						environmentDescriptor->bindingSets.Push(setDescriptor);
						BindingState state;
						{
							state.descriptorSet = setDescriptor;
							state.log = modelDescriptor.context->Log();
							state.lightingModelPath = &modelDescriptor.lightingModel;
						}
						bindingStates.Push(state);
					}
					BindingState* state = &bindingStates[bindingSetId];
					state->stage = stage;
					for (size_t bindingId = 0; bindingId < bindingSet.BindingCount(); bindingId++) {
						const Graphics::SPIRV_Binary::BindingInfo& binding = bindingSet.Binding(bindingId);
						if (binding.type < Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)
							includeBinding[static_cast<size_t>(binding.type)](state, &binding);
					}
				}
			};
			includeBinaryEntries(Graphics::PipelineStage::VERTEX, environmentDescriptor->vertexShader);
			includeBinaryEntries(Graphics::PipelineStage::FRAGMENT, environmentDescriptor->fragmentShader);

			// Make sure there are no malformed bindings:
			for (size_t i = 0; i < environmentDescriptor->bindingSets.Size(); i++) {
				const BindingSetDescriptor* setDescriptor = dynamic_cast<const BindingSetDescriptor*>(environmentDescriptor->bindingSets[i].operator->());
				if (setDescriptor->type != BindingSetDescriptor::Type::NORMAL && (
					setDescriptor->constantBuffers.Size() > 0u || setDescriptor->structuredBuffers.Size() > 0u ||
					setDescriptor->textureSamplers.Size() > 0u || setDescriptor->textureViews.Size() > 0u))
					modelDescriptor.context->Log()->Error("LightingModelPipelines - Environment binding set ", i, " contains bindless arrays, alongside bound resources; this is not supported <",
						modelDescriptor.lightingModel, ">! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}
		}

		inline static bool GenerateBindingSets(
			PipelineDescriptor* descriptor,
			const Descriptor& modelDescriptor,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings,
			const Graphics::SPIRV_Binary* vertexShader, const Graphics::SPIRV_Binary* fragmentShader) {
			static thread_local std::vector<Graphics::ShaderResourceBindings::BindingSetInfo> generatedBindings;
			static thread_local std::vector<Graphics::ShaderResourceBindings::ShaderModuleBindingSet> shaderBindingSets;

			auto cleanup = [&]() {
				generatedBindings.clear();
				shaderBindingSets.clear();
			};

			auto logErrorAndReturnFalse = [&](const char* text) {
				modelDescriptor.context->Log()->Error("LightingModelPipelines::Helpers::CreateGraphicsPipeline - ", text, " [File:", __FILE__, "; Line: ", __LINE__, "]");
				cleanup();
				return false;
			};

			// Generate binding sets:
			{
				shaderBindingSets.clear();
				auto addShaderBindingSets = [&](const Graphics::SPIRV_Binary* shader) {
					const Jimara::Graphics::PipelineStageMask stages = shader->ShaderStages();
					const size_t setCount = shader->BindingSetCount();
					for (size_t i = descriptor->bindingSets.Size(); i < setCount; i++)
						shaderBindingSets.push_back(Graphics::ShaderResourceBindings::ShaderModuleBindingSet(&shader->BindingSet(i), stages));
				};
				addShaderBindingSets(vertexShader);
				addShaderBindingSets(fragmentShader);
				if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(
					shaderBindingSets.data(), shaderBindingSets.size(), bindings,
					[&](const Graphics::ShaderResourceBindings::BindingSetInfo& info) { generatedBindings.push_back(info); }, modelDescriptor.context->Log()))
					return logErrorAndReturnFalse("Failed to generate shader binding sets for scene object!");
			}

			// Transfer generated bindings to the descriptor:
			const size_t initialBindingCount = descriptor->bindingSets.Size();
			for (size_t i = 0; i < generatedBindings.size(); i++) {
				const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo = generatedBindings[i];
				if (setInfo.setIndex < initialBindingCount)
					logErrorAndReturnFalse("Conflict with environment binding descriptor detected!");
				while (descriptor->bindingSets.Size() <= setInfo.setIndex)
					descriptor->bindingSets.Push(nullptr);
				descriptor->bindingSets[setInfo.setIndex] = setInfo.set;
			}

			// Make sure no sets are missing:
			for (size_t i = 0; i < descriptor->bindingSets.Size(); i++)
				if (descriptor->bindingSets[i] == nullptr)
					return logErrorAndReturnFalse("Incomplete set of shader binding set descriptors for the scene object!");

			cleanup();
			return true;
		}

		inline static Reference<GraphicsPipelineDescriptor> CreatePipelineDescriptor(
			GraphicsObjectDescriptor* graphicsObject,
			const Descriptor& modelDescriptor,
			Graphics::ShaderSet* shaderSet, Graphics::ShaderCache* shaderCache,
			PipelineDescriptor* environmentDescriptor) {

			auto logErrorAndReturnNull = [&](const char* text) {
				modelDescriptor.context->Log()->Error("LightingModelPipelines::Helpers::CreateGraphicsPipeline - ", text, " [File:", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			};

			// Get shader class:
			if (graphicsObject == nullptr) return logErrorAndReturnNull("Shader class missing!");
			const Graphics::ShaderClass* const shaderClass = graphicsObject->ShaderClass();
			if (shaderClass == nullptr) return logErrorAndReturnNull("Shader class missing!");

			// Get shader binaries:
			Reference<Graphics::SPIRV_Binary> vertexShader = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::VERTEX);
			if (vertexShader == nullptr) return logErrorAndReturnNull("Vertex shader not found!");
			Reference<Graphics::SPIRV_Binary> fragmentShader = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::FRAGMENT);
			if (fragmentShader == nullptr) return logErrorAndReturnNull("Fragment shader not found!");

			// Get/create shader modules:
			Reference<Graphics::Shader> vertexShaderInstance = shaderCache->GetShader(vertexShader);
			if (vertexShaderInstance == nullptr) logErrorAndReturnNull("Vertex shader instance could not be created!");
			Reference<Graphics::Shader> fragmentShaderInstance = shaderCache->GetShader(fragmentShader);
			if (fragmentShaderInstance == nullptr) logErrorAndReturnNull("Fragment shader instance could not be created!");

			// Transfer pipeline set descriptors from the environment pipeline:
			const Reference<GraphicsPipelineDescriptor> descriptor = Object::Instantiate<GraphicsPipelineDescriptor>();
			for (size_t i = 0; i < environmentDescriptor->bindingSets.Size(); i++)
				descriptor->bindingSets.Push(environmentDescriptor->bindingSets[i]);

			// Generate bindings:
			if(!GenerateBindingSets(descriptor, modelDescriptor, SceneObjectResourceBindings(graphicsObject, shaderClass, modelDescriptor.context->Graphics()->Device()), vertexShader, fragmentShader))
				logErrorAndReturnNull("Failed to generate pipeline descriptors!");

			// Set shaders and source object:
			{
				descriptor->graphicsObject = graphicsObject;
				descriptor->vertexShader = vertexShaderInstance;
				descriptor->fragmentShader = fragmentShaderInstance;
			}
			return descriptor;
		}


		// Per-instance data:
		struct PipelineDescPerObject {
			Reference<GraphicsObjectDescriptor> object;
			mutable Reference<GraphicsPipelineDescriptor> descriptor;
			mutable Reference<Graphics::GraphicsPipeline> pipeline;

			inline PipelineDescPerObject(GraphicsObjectDescriptor* obj = nullptr) : object(obj) {}
		};

		class InstanceData : public virtual ObjectCache<RenderPassDescriptor> {
		private:
			struct DataReference : public virtual Object {
				SpinLock lock;
				InstanceData* data = nullptr;
				virtual ~DataReference() { assert(data == nullptr); }
			};
			const Reference<DataReference> m_dataReference;
			const Reference<const LightingModelPipelines> m_pipelines;
			Reference<Graphics::RenderPass> m_renderPass;

			template<typename DescriptorReferenceType>
			inline void OnObjectsAddedLockless(const DescriptorReferenceType* objects, size_t count) {
				if (count <= 0) return;

				// Filter out descriptors:
				static thread_local std::vector<GraphicsObjectDescriptor*> descriptors;
				{
					descriptors.clear();
					const LayerMask layers = m_pipelines->m_modelDescriptor.layers;
					for (size_t i = 0; i < count; i++) {
						GraphicsObjectDescriptor* descriptor = objects[i];
						if (descriptor == nullptr) continue;
						else if (layers[descriptor->Layer()])
							descriptors.push_back(descriptor);
					}
				}

				// Add new objects and create pipeline descriptors:
				if (descriptors.size() > 0) {
					static thread_local std::vector<GraphicsObjectDescriptor*> discardedObjects;
					discardedObjects.clear();

					pipelineSet.Add(descriptors.data(), descriptors.size(), [&](const PipelineDescPerObject* added, size_t numAdded) {
#ifndef NDEBUG
						if (numAdded != descriptors.size())
							m_pipelines->m_modelDescriptor.context->Log()->Error(
								"LightingModelPipelines::Helpers::InstanceData::OnObjectsAddedLockless - (numAdded != descriptors.size())! [File: ", __FILE__, "; Line: ", __LINE__, "]");
#endif

						// __TODO__: This coul, in theory, benefit from some multithreading...
						for (size_t i = 0; i < numAdded; i++) {
							const PipelineDescPerObject* ptr = added + i;
							if (ptr->object->ShaderClass() == nullptr) {
								m_pipelines->m_modelDescriptor.context->Log()->Error(
									"LightingModelPipelines::Helpers::InstanceData::OnObjectsAddedLockless - Graphics object descriptor has no shader class (object will be discarded)! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								discardedObjects.push_back(ptr->object);
								continue;
							}
							ptr->descriptor = dynamic_cast<PipelineDescriptorCache*>(m_pipelines->m_pipelineDescriptorCache.operator->())->GetFor(
								ptr->object, m_pipelines->m_modelDescriptor, m_pipelines->m_shaderSet, m_pipelines->m_shaderCache,
								dynamic_cast<EnvironmentPipelineDescriptor*>(m_pipelines->m_environmentDescriptor.operator->()));
							if (ptr->descriptor == nullptr) {
								m_pipelines->m_modelDescriptor.context->Log()->Error(
									"LightingModelPipelines::Helpers::InstanceData::OnObjectsAddedLockless - Failed to get/generate pipeline descriptor (object will be discarded)! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								discardedObjects.push_back(ptr->object);
								continue;
							}
							ptr->pipeline = m_renderPass->CreateGraphicsPipeline(ptr->descriptor, m_pipelines->m_modelDescriptor.context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
							if (ptr->pipeline == nullptr) {
								m_pipelines->m_modelDescriptor.context->Log()->Error(
									"LightingModelPipelines::Helpers::InstanceData::OnObjectsAddedLockless - Failed to create a graphics pipeline (object will be discarded)! [File: ", __FILE__, "; Line: ", __LINE__, "]");
								discardedObjects.push_back(ptr->object);
							}
						}
						});

					if (!discardedObjects.empty()) {
						pipelineSet.Remove(discardedObjects.data(), discardedObjects.size());
						discardedObjects.clear();
					}
					descriptors.clear();
				}
			}

			template<typename DescriptorReferenceType>
			inline void OnObjectsRemovedLockless(const DescriptorReferenceType* objects, size_t count) {
				if (count <= 0) return;
				pipelineSet.Remove(objects, count);
			}

			inline void OnObjectsAdded(GraphicsObjectDescriptor*const * objects, size_t count) {
				std::unique_lock<std::shared_mutex> lock(pipelineSetLock);
				OnObjectsAddedLockless(objects, count);
			}

			inline void OnObjectsRemoved(GraphicsObjectDescriptor* const* objects, size_t count) {
				std::unique_lock<std::shared_mutex> lock(pipelineSetLock);
				OnObjectsRemovedLockless(objects, count);
			}

			void Subscribe() {
				// __TODO__: Subscribe to events:
				m_pipelines->m_graphicsObjects->OnAdded() += Callback(&InstanceData::OnObjectsAdded, this);
				m_pipelines->m_graphicsObjects->OnRemoved() += Callback(&InstanceData::OnObjectsRemoved, this);
			}

			void Unsubscribe() {
				// __TODO__: Unsubscribe from events:
				m_pipelines->m_graphicsObjects->OnAdded() -= Callback(&InstanceData::OnObjectsAdded, this);
				m_pipelines->m_graphicsObjects->OnRemoved() -= Callback(&InstanceData::OnObjectsRemoved, this);
			}

		public:
			std::shared_mutex pipelineSetLock;
			ObjectSet<GraphicsObjectDescriptor, PipelineDescPerObject> pipelineSet;




			inline InstanceData(const LightingModelPipelines* pipelines)
				: m_dataReference(Object::Instantiate<DataReference>())
				, m_pipelines(pipelines) {
				m_pipelines->m_modelDescriptor.context->StoreDataObject(this);
				m_dataReference->data = this;
			}

			inline virtual ~InstanceData() {
				Unsubscribe();
			}

			inline Object* GetReference()const { return m_dataReference; }

			inline void Initailize(Graphics::RenderPass* renderPass) {
				m_renderPass = renderPass;
				Subscribe();
				{
					std::unique_lock<std::shared_mutex> lock(pipelineSetLock);
					std::vector<Reference<GraphicsObjectDescriptor>> allObjects;
					m_pipelines->m_graphicsObjects->GetAll([&](GraphicsObjectDescriptor* descriptor) { allObjects.push_back(descriptor); });
					OnObjectsAddedLockless(allObjects.data(), allObjects.size());
				}
			}

			inline void Dispose() {
				m_pipelines->m_modelDescriptor.context->EraseDataObject(this);
				Unsubscribe();
				{
					std::unique_lock<std::shared_mutex> lock(pipelineSetLock);
					pipelineSet.Clear();
				}
			}

			inline static Reference<InstanceData> GetData(Object* reference) {
				DataReference* ref = dynamic_cast<DataReference*>(reference);
				std::unique_lock<SpinLock> lock(ref->lock);
				Reference<InstanceData> data = ref->data;
				return data;
			}

		protected:
			inline virtual void OnOutOfScope()const override {
				std::unique_lock<SpinLock> lock(m_dataReference->lock);
				if (RefCount() > 0) return;
				else m_dataReference->data = nullptr;
				Object::OnOutOfScope();
			}
		};





		// Caches:
		class Cache : public virtual ObjectCache<Descriptor> {
		private:
#pragma warning(disable: 4250)
			class CachedObject : public virtual LightingModelPipelines, public virtual ObjectCache<Descriptor>::StoredObject {
			public:
				inline CachedObject(const Descriptor& descriptor) : LightingModelPipelines(descriptor) {}
				inline virtual ~CachedObject() {}
			};
#pragma warning(default: 4250)

		public:
			inline static Reference<LightingModelPipelines> GetFor(const Descriptor& descriptor) {
				static Cache cache;
				return cache.GetCachedOrCreate(descriptor, false, [&]() {
					return Object::Instantiate<CachedObject>(descriptor);
				});
			}
		};

		class PipelineDescriptorCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline Reference<GraphicsPipelineDescriptor> GetFor(
				GraphicsObjectDescriptor* graphicsObject,
				const Descriptor& modelDescriptor,
				Graphics::ShaderSet* shaderSet, Graphics::ShaderCache* shaderCache,
				PipelineDescriptor* environmentDescriptor) {
				return GetCachedOrCreate(graphicsObject, false, [&]() {
					return CreatePipelineDescriptor(graphicsObject, modelDescriptor, shaderSet, shaderCache, environmentDescriptor);
					});
			}
		};

		class InstanceCache : public virtual ObjectCache<RenderPassDescriptor> {
		private:
#pragma warning(disable: 4250)
			class CachedObject : public virtual Instance, public virtual ObjectCache<RenderPassDescriptor>::StoredObject {
			public:
				inline CachedObject(const RenderPassDescriptor& descriptor, const LightingModelPipelines* pipelines) : Instance(descriptor, pipelines) {}
				inline virtual ~CachedObject() {}
			};
#pragma warning(default: 4250)

		public:
			inline Reference<Instance> GetFor(const RenderPassDescriptor& descriptor, const LightingModelPipelines* pipelines) {
				return GetCachedOrCreate(descriptor, false, [&]() { return Object::Instantiate<CachedObject>(descriptor, pipelines); });
			}
		};
	};





	// LightingModelPipelines public interface:
	LightingModelPipelines::LightingModelPipelines(const Descriptor& descriptor)
		: m_modelDescriptor(descriptor)
		, m_shaderSet(descriptor.context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet(descriptor.lightingModel))
		, m_shaderCache(Graphics::ShaderCache::ForDevice(descriptor.context->Graphics()->Device()))
		, m_graphicsObjects(GraphicsObjectDescriptor::Set::GetInstance(descriptor.context))
		, m_environmentDescriptor(Object::Instantiate<Helpers::EnvironmentPipelineDescriptor>())
		, m_pipelineDescriptorCache(Object::Instantiate<Helpers::PipelineDescriptorCache>())
		, m_instanceCache(Object::Instantiate<Helpers::InstanceCache>()) {
		if (m_shaderSet == nullptr)
			m_modelDescriptor.context->Log()->Error("LightingModelPipelines -  Failed to load shader set for '", descriptor.lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if (m_graphicsObjects == nullptr)
			m_modelDescriptor.context->Log()->Fatal("LightingModelPipelines - Internal error: Failed to get graphics object collection! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		Helpers::GenerateEnvironmentPipeline(m_modelDescriptor, m_shaderSet, dynamic_cast<Helpers::EnvironmentPipelineDescriptor*>(m_environmentDescriptor.operator->()));
	}

	LightingModelPipelines::~LightingModelPipelines() {
	}

	Reference<LightingModelPipelines> LightingModelPipelines::Get(const Descriptor& descriptor) {
		if (descriptor.context == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(descriptor);
	}

	Reference<LightingModelPipelines::Instance> LightingModelPipelines::GetInstance(const RenderPassDescriptor& renderPassInfo)const {
		return dynamic_cast<Helpers::InstanceCache*>(m_instanceCache.operator->())->GetFor(renderPassInfo, this);
	}

	Reference<Graphics::Pipeline> LightingModelPipelines::CreateEnvironmentPipeline(const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings)const {
		const Helpers::EnvironmentPipelineDescriptor* environment = dynamic_cast<Helpers::EnvironmentPipelineDescriptor*>(m_environmentDescriptor.operator->());
		const Reference<Helpers::PipelineDescriptor> descriptor = Object::Instantiate<Helpers::PipelineDescriptor>();
		if (!Helpers::GenerateBindingSets(descriptor, m_modelDescriptor, bindings, environment->vertexShader, environment->fragmentShader)) {
			m_modelDescriptor.context->Log()->Error("LightingModelPipelines::CreateEnvironmentPipeline - Failed to generate bindings for the environment pipeline!");
			return nullptr;
		}
		else return m_modelDescriptor.context->Graphics()->Device()->CreateEnvironmentPipeline(
			descriptor, m_modelDescriptor.context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
	}





	// Instance:
	LightingModelPipelines::Instance::Instance(const RenderPassDescriptor& renderPassInfo, const LightingModelPipelines* pipelines)
		: m_pipelines(pipelines)
		, m_renderPass(pipelines->m_modelDescriptor.context->Graphics()->Device()->CreateRenderPass(
			renderPassInfo.sampleCount, renderPassInfo.colorAttachmentFormats.Size(), renderPassInfo.colorAttachmentFormats.Data(),
			renderPassInfo.depthFormat, renderPassInfo.renderPassFlags))
		, m_instanceDataReference([&]()->Reference<Object> {
		Reference<Helpers::InstanceData> data = Object::Instantiate<Helpers::InstanceData>(pipelines);
		Reference<Object> rv = data->GetReference();
		return rv;
			}()) {
		if (m_renderPass == nullptr)
			m_pipelines->m_modelDescriptor.context->Log()->Fatal("LightingModelPipelines::Instance::Instance - Failed to create the render pass!");
		Reference<Helpers::InstanceData> instanceData = Helpers::InstanceData::GetData(m_instanceDataReference);
		if (instanceData == nullptr)
			m_pipelines->m_modelDescriptor.context->Log()->Fatal("LightingModelPipelines::Instance::Instance - Internal error: Instance data not found!");
		else instanceData->Initailize(m_renderPass);
	}

	LightingModelPipelines::Instance::~Instance() {
		Reference<Helpers::InstanceData> instanceData = Helpers::InstanceData::GetData(m_instanceDataReference);
		if (instanceData != nullptr) instanceData->Dispose();
	}

	Graphics::RenderPass* LightingModelPipelines::Instance::RenderPass()const { return m_renderPass; }





	// Instance reader:
	LightingModelPipelines::Reader::Reader(const Instance* instance) 
		: Reader([&]() -> Reference<Object> {
		if (instance == nullptr) return nullptr;
		Reference<Object> instanceData = Helpers::InstanceData::GetData(instance->m_instanceDataReference);
		return instanceData;
			}()) {}

	LightingModelPipelines::Reader::Reader(Reference<Object> data) 
		: m_lock([&]()->std::shared_mutex& {
		Helpers::InstanceData* instanceData = dynamic_cast<Helpers::InstanceData*>(data.operator->());
		if (instanceData == nullptr) {
			static std::shared_mutex lock;
			return lock;
		}
		else return instanceData->pipelineSetLock;
			}())
		, m_data(data) {
		Helpers::InstanceData* instanceData = dynamic_cast<Helpers::InstanceData*>(data.operator->());
		if (instanceData != nullptr) {
			m_count = instanceData->pipelineSet.Size();
			const Helpers::PipelineDescPerObject* pipelineData = instanceData->pipelineSet.Data();
			m_pipelineData = reinterpret_cast<const void*>(pipelineData);
		}
	}

	LightingModelPipelines::Reader::~Reader() {}

	size_t LightingModelPipelines::Reader::PipelineCount()const { return m_count; }

	Graphics::GraphicsPipeline* LightingModelPipelines::Reader::Pipeline(size_t index)const {
		return reinterpret_cast<const Helpers::PipelineDescPerObject*>(m_pipelineData)[index].pipeline;
	}

	GraphicsObjectDescriptor* LightingModelPipelines::Reader::GraphicsObject(size_t index)const {
		return reinterpret_cast<const Helpers::PipelineDescPerObject*>(m_pipelineData)[index].object;
	}





	// Descriptors:
	bool LightingModelPipelines::Descriptor::operator==(const Descriptor& other)const {
		return context == other.context && layers == other.layers && lightingModel == other.lightingModel;
	}
	bool LightingModelPipelines::Descriptor::operator<(const Descriptor& other)const {
		if (context < other.context) return true;
		else if (context > other.context) return false;
		
		if (layers < other.layers) return true;
		else if (layers > other.layers) return false;

		if (lightingModel < other.lightingModel) return true;
		else if (lightingModel > other.lightingModel) return false;

		return false;
	}

	bool LightingModelPipelines::RenderPassDescriptor::operator==(const RenderPassDescriptor& other)const {
		if (sampleCount != other.sampleCount || colorAttachmentFormats.Size() != other.colorAttachmentFormats.Size() ||
			depthFormat != other.depthFormat || renderPassFlags != other.renderPassFlags) return false;
		auto data = colorAttachmentFormats.Data();
		auto otherData = other.colorAttachmentFormats.Data();
		for (size_t i = 0; i < colorAttachmentFormats.Size(); i++)
			if (data[i] != otherData[i]) return false;
		return true;
	}
	bool LightingModelPipelines::RenderPassDescriptor::operator<(const RenderPassDescriptor& other)const {
		if (sampleCount < other.sampleCount) return true;
		else if (sampleCount > other.sampleCount) return false;

		if (colorAttachmentFormats.Size() < other.colorAttachmentFormats.Size()) return true;
		else if (colorAttachmentFormats.Size() > other.colorAttachmentFormats.Size()) return false;

		if (depthFormat < other.depthFormat) return true;
		else if (depthFormat > other.depthFormat) return false;

		if (renderPassFlags < other.renderPassFlags) return true;
		else if (renderPassFlags > other.renderPassFlags) return false;

		auto data = colorAttachmentFormats.Data();
		auto otherData = other.colorAttachmentFormats.Data();
		for (size_t i = 0; i < colorAttachmentFormats.Size(); i++) {
			auto format = data[i];
			auto otherFormat = otherData[i];
			if (format < otherFormat) return true;
			else if (format > otherFormat) return false;
		}

		return false;
	}
}

namespace std {
	size_t hash<Jimara::LightingModelPipelines::Descriptor>::operator()(const Jimara::LightingModelPipelines::Descriptor& descriptor)const {
		return Jimara::MergeHashes(
			hash<Jimara::Scene::LogicContext*>()(descriptor.context),
			Jimara::MergeHashes(
				hash<Jimara::LayerMask>()(descriptor.layers),
				hash<Jimara::OS::Path>()(descriptor.lightingModel)));
	}
	size_t hash<Jimara::LightingModelPipelines::RenderPassDescriptor>::operator()(const Jimara::LightingModelPipelines::RenderPassDescriptor& descriptor)const {
		auto hashOf = [](auto val) { return hash<decltype(val)>()(val); };
		size_t hash = Jimara::MergeHashes(
			Jimara::MergeHashes(hashOf(descriptor.sampleCount), hashOf(descriptor.colorAttachmentFormats.Size())),
			Jimara::MergeHashes(hashOf(descriptor.depthFormat), hashOf(descriptor.renderPassFlags)));
		auto data = descriptor.colorAttachmentFormats.Data();
		for (size_t i = 0; i < descriptor.colorAttachmentFormats.Size(); i++)
			hash = Jimara::MergeHashes(hash, hashOf(data[i]));
		return hash;
	}
}
