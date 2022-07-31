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

#pragma warning(disable: 4250)
		struct GraphicsPipelineDescriptor : public virtual PipelineDescriptor, public virtual Graphics::GraphicsPipeline::Descriptor {
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

		inline static void GenerateEnvironmentPipeline(const Descriptor& modelDescriptor, Graphics::ShaderSet* shaderSet, PipelineDescriptor* environmentDescriptor) {
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
			auto includeBinaryEntries = [&](Graphics::PipelineStage stage) {
				Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&blankShader, stage);
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
			includeBinaryEntries(Graphics::PipelineStage::FRAGMENT);
			includeBinaryEntries(Graphics::PipelineStage::VERTEX);

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

		inline static Reference<GraphicsPipelineDescriptor> CreatePipelineDescriptor(
			GraphicsObjectDescriptor* graphicsObject,
			const Descriptor& modelDescriptor,
			Graphics::ShaderSet* shaderSet, Graphics::ShaderCache* shaderCache,
			PipelineDescriptor* environmentDescriptor) {
			static thread_local std::vector<Graphics::ShaderResourceBindings::BindingSetInfo> generatedBindings;
			static thread_local std::vector<Graphics::ShaderResourceBindings::ShaderModuleBindingSet> shaderBindingSets;

			auto cleanup = [&]() {
				generatedBindings.clear();
				shaderBindingSets.clear();
			};

			auto logErrorAndReturnNull = [&](const char* text) {
				modelDescriptor.context->Log()->Error("LightingModelPipelines::Helpers::CreateGraphicsPipeline - ", text, " [File:", __FILE__, "; Line: ", __LINE__, "]");
				cleanup();
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

			// Create pipeline set descriptors:
			{
				shaderBindingSets.clear();
				auto addShaderBindingSets = [&](const Graphics::SPIRV_Binary* shader, const Jimara::Graphics::PipelineStageMask stages) {
					const size_t setCount = shader->BindingSetCount();
					for (size_t i = environmentDescriptor->bindingSets.Size(); i < setCount; i++)
						shaderBindingSets.push_back(Graphics::ShaderResourceBindings::ShaderModuleBindingSet(&shader->BindingSet(i), stages));
				};
				addShaderBindingSets(vertexShader, Graphics::StageMask(Graphics::PipelineStage::VERTEX));
				addShaderBindingSets(fragmentShader, Graphics::StageMask(Graphics::PipelineStage::FRAGMENT));
				if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(
					shaderBindingSets.data(), shaderBindingSets.size(), SceneObjectResourceBindings(graphicsObject, shaderClass, modelDescriptor.context->Graphics()->Device()),
					[&](const Graphics::ShaderResourceBindings::BindingSetInfo& info) { generatedBindings.push_back(info); }, modelDescriptor.context->Log()))
					return logErrorAndReturnNull("Failed to generate shader binding sets for scene object!");
			}

			// Transfer pipeline set descriptors from the environment pipeline:
			const Reference<GraphicsPipelineDescriptor> descriptor = Object::Instantiate<GraphicsPipelineDescriptor>();
			for (size_t i = 0; i < environmentDescriptor->bindingSets.Size(); i++)
				descriptor->bindingSets.Push(environmentDescriptor->bindingSets[i]);

			// Transfer generated bindings to the descriptor:
			for (size_t i = 0; i < generatedBindings.size(); i++) {
				const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo = generatedBindings[i];
				if (setInfo.setIndex < environmentDescriptor->bindingSets.Size()) 
					logErrorAndReturnNull("Conflict with environment binding descriptor detected!");
				while (descriptor->bindingSets.Size() <= setInfo.setIndex)
					descriptor->bindingSets.Push(nullptr);
				descriptor->bindingSets[setInfo.setIndex] = setInfo.set;
			}

			// Make sure no sets are missing:
			for (size_t i = 0; i < descriptor->bindingSets.Size(); i++) 
				if (descriptor->bindingSets[i] == nullptr) 
					return logErrorAndReturnNull("Incomplete set of shader binding set descriptors for the scene object!");

			// Set shaders and source object:
			{
				descriptor->graphicsObject = graphicsObject;
				descriptor->vertexShader = vertexShader;
				descriptor->fragmentShader = fragmentShader;
			}

			cleanup();
			return descriptor;
		}


		// Pipeline descriptor collection:
		class Data : public virtual ObjectCache<RenderPassDescriptor> {
		private:
			struct DataReference : public virtual Object {
				SpinLock lock;
				Data* data;

				DataReference(Data* ref) : data(data) {}

				virtual ~DataReference() { assert(data == nullptr); }
			};
			const Reference<DataReference> m_dataReference;

		public:
			const Descriptor modelDescriptor;
			const Reference<Graphics::ShaderSet> shaderSet;
			const Reference<Graphics::ShaderCache> shaderCache;
			const Reference<GraphicsObjectDescriptor::Set> graphicsObjects;
			const Reference<PipelineDescriptor> environmentDescriptor;

			inline Data(const Descriptor& descriptor) 
				: m_dataReference(Object::Instantiate<DataReference>(this))
				, modelDescriptor(descriptor)
				, shaderSet(descriptor.context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet(descriptor.lightingModel))
				, shaderCache(Graphics::ShaderCache::ForDevice(descriptor.context->Graphics()->Device()))
				, graphicsObjects(GraphicsObjectDescriptor::Set::GetInstance(descriptor.context))
				, environmentDescriptor(Object::Instantiate<PipelineDescriptor>()) {
				if (shaderSet == nullptr)
					modelDescriptor.context->Log()->Error("LightingModelPipelines -  Failed to load shader set for '", descriptor.lightingModel, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				if (graphicsObjects == nullptr)
					modelDescriptor.context->Log()->Fatal("LightingModelPipelines - Internal error: Failed to get graphics object collection! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				GenerateEnvironmentPipeline(modelDescriptor, shaderSet, environmentDescriptor);
				modelDescriptor.context->StoreDataObject(this);
			}

			inline virtual ~Data() {
				assert(m_dataReference == nullptr);
			}

			inline Object* GetReference()const { return m_dataReference; }

			inline void Dispose() {
				modelDescriptor.context->EraseDataObject(this);
			}

			inline static Reference<Data> GetData(Object* reference) {
				DataReference* ref = dynamic_cast<DataReference*>(reference);
				std::unique_lock<SpinLock> lock(ref->lock);
				Reference<Data> data = ref->data;
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





		// Instance cache:
		class Cache : public virtual ObjectCache<Descriptor> {
		private:
#pragma warning(disable: 4250)
			class CachedObject : public virtual LightingModelPipelines, public virtual ObjectCache<Descriptor>::StoredObject {
			public:
				inline CachedObject(Scene::LogicContext* context, Object* dataReference) : LightingModelPipelines(context, dataReference) {}
			};
#pragma warning(default: 4250)

		public:
			inline static Reference<LightingModelPipelines> GetFor(const Descriptor& descriptor) {
				static Cache cache;
				return cache.GetCachedOrCreate(descriptor, false, [&]() {
					Reference<Data> data = Object::Instantiate<Data>(descriptor);
					return Object::Instantiate<CachedObject>(descriptor.context, data->GetReference());
				});
			}
		};
	};





	// LightingModelPipelines public interface:
	LightingModelPipelines::LightingModelPipelines(Scene::LogicContext* context, Object* dataReference)
		: m_context(context), m_dataReference(dataReference) {}

	LightingModelPipelines::~LightingModelPipelines() {
		Reference<Helpers::Data> data = Helpers::Data::GetData(m_dataReference);
		if (data != nullptr) data->Dispose();
	}

	Reference<LightingModelPipelines> LightingModelPipelines::Get(const Descriptor& descriptor) {
		if (descriptor.context == nullptr) return nullptr;
		else return Helpers::Cache::GetFor(descriptor);
	}

	Reference<LightingModelPipelines::Instance> LightingModelPipelines::GetInstance(const RenderPassDescriptor& renderPassInfo)const {
		m_context->Log()->Error("LightingModelPipelines::GetInstance - Not yet implemented!");
		return nullptr;
	}





	// Instance:
	LightingModelPipelines::Instance::Instance(const RenderPassDescriptor& renderPassInfo) {
		// __TODO__: Implement this crap!
	}

	LightingModelPipelines::Instance::~Instance() {
		// __TODO__: Implement this crap!
	}

	Reference<Graphics::Pipeline> LightingModelPipelines::Instance::CreateEnvironmentPipeline(const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings)const {
		return nullptr;
	}

	Graphics::RenderPass* LightingModelPipelines::Instance::RenderPass()const { return m_renderPass; }





	// Instance reader:
	LightingModelPipelines::Reader::Reader(const Instance* instance) {
		// __TODO__: Implement this crap!
	}

	LightingModelPipelines::Reader::~Reader() {
		// __TODO__: Implement this crap!
	}

	size_t LightingModelPipelines::Reader::PipelineCount()const {
		// __TODO__: Implement this crap!
		return 0u;
	}

	Graphics::GraphicsPipeline* LightingModelPipelines::Reader::Pipeline(size_t index)const {
		// __TODO__: Implement this crap!
		return nullptr;
	}

	GraphicsObjectDescriptor* LightingModelPipelines::Reader::GraphicsObject(size_t index)const {
		// __TODO__: Implement this crap!
		return nullptr;
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
