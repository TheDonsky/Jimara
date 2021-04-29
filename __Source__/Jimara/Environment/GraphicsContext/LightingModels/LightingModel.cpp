#include "LightingModel.h"


#pragma warning(disable: 4250)
namespace Jimara {
	namespace {
		struct LitShaderModules {
			LightingModel::LitShaderModules modules;
			Reference<Graphics::Shader> vertex;
			Reference<Graphics::Shader> fragment;

			LitShaderModules(LightingModel* model, const SceneObjectDescriptor* object) {
				modules = model->GetShaderBinaries(object->Material()->ShaderId());
				Graphics::ShaderCache* cache = Graphics::ShaderCache::ForDevice(model->Device());
				vertex = cache->GetShader(modules.vertexShader);
				fragment = cache->GetShader(modules.fragmentShader);
			}
		};

		class PipelineBindingSetDescriptor : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
		public:
			typedef std::pair<const Graphics::SPIRV_Binary::BindingSetInfo*, Graphics::PipelineStageMask> SetInformation;

		private:
			std::vector<BindingInfo> m_bindingInfos;
			size_t m_constantBufferCount = 0;
			BindingInfo* m_constantBufferInfos = nullptr;
			size_t m_structuredBufferCount = 0;
			BindingInfo* m_structuredBufferInfos = nullptr;
			size_t m_textureSamplerCount = 0;
			BindingInfo* m_textureSamplerInfos = nullptr;

		protected:
			inline PipelineBindingSetDescriptor(const SetInformation* setInfos, size_t infoCount, OS::Logger* log) {
				// Compile all existing bindings:
				typedef std::pair<const Graphics::SPIRV_Binary::BindingInfo*, Graphics::PipelineStageMask> BindingInformation;
				static thread_local std::unordered_map<size_t, BindingInformation> bindingToInfo;
				bindingToInfo.clear();
				for (size_t infoId = 0; infoId < infoCount; infoId++) {
					const SetInformation& setInfo = setInfos[infoId];
					const size_t bindingCount = setInfo.first->BindingCount();
					for (size_t bindingIndex = 0; bindingIndex < bindingCount; bindingIndex++) {
						const Graphics::SPIRV_Binary::BindingInfo* bindingInfo = &setInfo.first->Binding(bindingIndex);
						std::unordered_map<size_t, BindingInformation>::iterator it = bindingToInfo.find(bindingInfo->binding);
						if (it == bindingToInfo.end())
							bindingToInfo[bindingInfo->binding] = std::make_pair(bindingInfo, setInfo.second);
						else {
							it->second.second |= setInfo.second;
							if (it->second.first->type == Graphics::SPIRV_Binary::BindingInfo::Type::UNKNOWN)
								it->second.first = bindingInfo;
							else if ((bindingInfo->type != Graphics::SPIRV_Binary::BindingInfo::Type::UNKNOWN) && (it->second.first->type != bindingInfo->type))
								log->Fatal("LightingModel:PipelineBindingSetDescriptor - Type mismatch on binding ", bindingInfo->binding, " of set ", bindingInfo->set, "!");
						}
					}
				}

				static const uint8_t BINDING_TYPE_COUNT = static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT);

				// Count binding per type:
				{
					m_constantBufferCount = m_structuredBufferCount = m_textureSamplerCount = 0;
					typedef void(*RecordTypeCountFn)(PipelineBindingSetDescriptor*);
					static const RecordTypeCountFn* RECORD_TYPE_COUNT_FUNCTIONS = []() -> RecordTypeCountFn* {
						static RecordTypeCountFn functions[BINDING_TYPE_COUNT];
						static const RecordTypeCountFn defaultFn = [](PipelineBindingSetDescriptor*) {};
						for (uint8_t i = 0; i < BINDING_TYPE_COUNT; i++) functions[i] = defaultFn;
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = [](PipelineBindingSetDescriptor* self) { self->m_constantBufferCount++; };
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = [](PipelineBindingSetDescriptor* self) { self->m_structuredBufferCount++; };
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = [](PipelineBindingSetDescriptor* self) { self->m_textureSamplerCount++; };
						return functions;
					}();
					for (std::unordered_map<size_t, BindingInformation>::const_iterator it = bindingToInfo.begin(); it != bindingToInfo.end(); ++it) {
						uint8_t type = static_cast<uint8_t>(it->second.first->type);
						if (type < BINDING_TYPE_COUNT) RECORD_TYPE_COUNT_FUNCTIONS[type](this);
					}
				}

				// Fill Binding infos:
				{
					m_bindingInfos.resize(m_constantBufferCount + m_structuredBufferCount + m_textureSamplerCount);
					auto setBindingAddrs = [&]() {
						m_constantBufferInfos = m_bindingInfos.data();
						m_structuredBufferInfos = m_constantBufferInfos + m_constantBufferCount;
						m_textureSamplerInfos = m_structuredBufferInfos + m_structuredBufferCount;
					};
					setBindingAddrs();
					typedef void(*AddBindingInfoFn)(PipelineBindingSetDescriptor*, const BindingInformation&);
					static const AddBindingInfoFn* FILL_BINDING_INFO = []() -> AddBindingInfoFn* {
						static AddBindingInfoFn functions[BINDING_TYPE_COUNT];
						static const AddBindingInfoFn defaultFn = [](PipelineBindingSetDescriptor*, const BindingInformation&) {};
						for (uint8_t i = 0; i < BINDING_TYPE_COUNT; i++) functions[i] = defaultFn;
						static const auto fillBindingDetails = [](const BindingInformation& binding, BindingInfo* info) {
							info->binding = static_cast<uint32_t>(binding.first->binding);
							info->stages = binding.second;
						};
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = [](PipelineBindingSetDescriptor* self, const BindingInformation& info) {
							fillBindingDetails(info, self->m_constantBufferInfos);
							self->m_constantBufferInfos++;
						};
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = [](PipelineBindingSetDescriptor* self, const BindingInformation& info) {
							fillBindingDetails(info, self->m_structuredBufferInfos);
							self->m_structuredBufferInfos++;
						};
						functions[static_cast<uint8_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = [](PipelineBindingSetDescriptor* self, const BindingInformation& info) {
							fillBindingDetails(info, self->m_textureSamplerInfos);
							self->m_textureSamplerInfos++;
						};
						return functions;
					}();
					for (std::unordered_map<size_t, BindingInformation>::const_iterator it = bindingToInfo.begin(); it != bindingToInfo.end(); ++it) {
						const BindingInformation& info = it->second;
						uint8_t type = static_cast<uint8_t>(info.first->type);
						if (type >= BINDING_TYPE_COUNT) FILL_BINDING_INFO[type](this, info);
					}
					setBindingAddrs();
				}
			}

		public:
			inline virtual size_t ConstantBufferCount()const override { return m_constantBufferCount; }
			inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return m_constantBufferInfos[index]; }

			inline virtual size_t StructuredBufferCount()const override { return m_structuredBufferCount; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return m_structuredBufferInfos[index]; }

			inline virtual size_t TextureSamplerCount()const override { return m_textureSamplerCount; }
			inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return m_textureSamplerInfos[index]; }
		};

		class EnvironmentPipelineBindingSetDescriptor : public virtual PipelineBindingSetDescriptor {
		public:
			inline virtual bool SetByEnvironment()const override { return false; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return nullptr; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return nullptr; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return nullptr; }
		};

		class ObjectPipelineBindingSetDescriptor : public virtual PipelineBindingSetDescriptor {
		public:
			inline virtual bool SetByEnvironment()const override { return false; }
		};

		class ObjectPipelineDescriptor : public virtual Graphics::GraphicsPipeline::Descriptor {
		private:
			const Reference<LightingModel> m_model;
			const Reference<const SceneObjectDescriptor> m_object;
			const LitShaderModules m_modules;

		public:
			inline ObjectPipelineDescriptor(LightingModel* model, const SceneObjectDescriptor* object)
				: m_model(model), m_object(object), m_modules(model, object) {
				// __TODO__: Extract binding set data..
			}

			virtual size_t BindingSetCount()const override { return 0; }
			virtual const BindingSetDescriptor* BindingSet(size_t index)const override { return nullptr; }

			inline virtual Reference<Graphics::Shader> VertexShader() override { return m_modules.vertex; }
			inline virtual Reference<Graphics::Shader> FragmentShader() override { return m_modules.fragment; }

			inline virtual size_t VertexBufferCount() override { return m_object->VertexBufferCount(); }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override { return m_object->VertexBuffer(index); }

			inline virtual size_t InstanceBufferCount() override { return m_object->InstanceBufferCount(); }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override { return m_object->InstanceBuffer(index); }
			
			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override { return m_object->IndexBuffer(); }

			inline virtual size_t IndexCount() override { return m_object->IndexCount(); }
			inline virtual size_t InstanceCount() override { return m_object->InstanceCount(); }
		};

		class ObjectPipelineDescriptorCache
			: public virtual ObjectCache<Reference<const SceneObjectDescriptor>>
			, public virtual ObjectCache<Reference<Graphics::GraphicsDevice>>::StoredObject {
		public:
			class CacheOfCaches : public virtual ObjectCache<Reference<Graphics::GraphicsDevice>> {
			public:
				inline static Reference<ObjectPipelineDescriptorCache> Instance(Graphics::GraphicsDevice* device) {
					static CacheOfCaches cache;
					return cache.GetCachedOrCreate(device, false
						, [&]() -> Reference<ObjectPipelineDescriptorCache> { return Object::Instantiate<ObjectPipelineDescriptorCache>(); });
				}
			};

			Reference<Graphics::GraphicsPipeline::Descriptor> GetDescriptor(LightingModel* model, const SceneObjectDescriptor* descriptor) {
				return GetCachedOrCreate(descriptor, false
					, [&]()->Reference<Graphics::GraphicsPipeline::Descriptor> { return Object::Instantiate<ObjectPipelineDescriptor>(model, descriptor); });
			}
		};
	}

	Reference<Graphics::GraphicsPipeline::Descriptor> LightingModel::GetPipelineDescriptor(const SceneObjectDescriptor* descriptor) {
		return dynamic_cast<ObjectPipelineDescriptorCache*>(m_cache.operator->())->GetDescriptor(this, descriptor);
	}

	Graphics::GraphicsDevice* LightingModel::Device()const { return m_device; }

	LightingModel::LightingModel(Graphics::GraphicsDevice* device) 
		: m_device(device), m_cache(ObjectPipelineDescriptorCache::CacheOfCaches::Instance(device)) { }
}
#pragma warning(default: 4250)
