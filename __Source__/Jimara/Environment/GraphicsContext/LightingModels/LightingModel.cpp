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
