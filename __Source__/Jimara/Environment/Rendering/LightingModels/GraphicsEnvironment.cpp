#include "GraphicsEnvironment.h"

#ifndef max
#define max std::max
#endif
#ifndef min
#define min std::min
#endif

namespace Jimara {
	namespace {
		struct EnvironmentPipelineSetDescriptors : public virtual Object {
			class DescriptorInstance : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			private:
				mutable Reference<EnvironmentPipelineSetDescriptors> owner;
				friend struct EnvironmentPipelineSetDescriptors;

			public:
				Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> base;

				inline virtual bool SetByEnvironment()const override { return true; }

				inline virtual size_t ConstantBufferCount()const override { return base->ConstantBufferCount(); }
				inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return base->ConstantBufferInfo(index); }
				inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return nullptr; }
				
				inline virtual size_t StructuredBufferCount()const override { return base->StructuredBufferCount(); }
				inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return base->StructuredBufferInfo(index); }
				inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return nullptr; }

				inline virtual size_t TextureSamplerCount()const override { return base->TextureSamplerCount(); }
				inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return base->TextureSamplerInfo(index); }
				inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return nullptr; }

				inline virtual bool IsBindlessArrayBufferArray()const override { return base->IsBindlessArrayBufferArray(); }
				inline virtual bool IsBindlessTextureSamplerArray()const override { return base->IsBindlessTextureSamplerArray(); }

			protected:
				inline virtual void OnOutOfScope()const override { owner = nullptr; }
			};

			std::vector<DescriptorInstance> instances;

			inline EnvironmentPipelineSetDescriptors(size_t count) : instances(count) {
				for (size_t i = 0; i < count; i++) {
					DescriptorInstance& instance = instances[i];
					instance.ReleaseRef();
					instance.owner = this;
				}
			}
		};
	}

	Reference<GraphicsEnvironment> GraphicsEnvironment::Create(
		Graphics::ShaderSet* shaderSet,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& environmentBindings,
		const GraphicsObjectDescriptor* sampleObject,
		Graphics::GraphicsDevice* device) {
		
		auto logErrorAndReturnNull = [&](const char* text) {
			device->Log()->Error("GraphicsEnvironment::Create - ", text);
			return nullptr;
		};

		if (sampleObject == nullptr) return logErrorAndReturnNull("sampleObject not provided!");
		const Reference<const Graphics::ShaderClass> shader = sampleObject->ShaderClass();
		if (shader == nullptr) return logErrorAndReturnNull("sampleObject has no shader!");

		Reference<Graphics::SPIRV_Binary> vertexShader = shaderSet->GetShaderModule(shader, Graphics::PipelineStage::VERTEX);
		if (vertexShader == nullptr) return logErrorAndReturnNull("Vertex shader not found!");
		
		Reference<Graphics::SPIRV_Binary> fragmentShader = shaderSet->GetShaderModule(shader, Graphics::PipelineStage::FRAGMENT);
		if (fragmentShader == nullptr) return logErrorAndReturnNull("Fragment shader not found!");

		static thread_local std::vector<Graphics::ShaderResourceBindings::ShaderModuleBindingSet> environmentBindingSets;
		environmentBindingSets.clear();
		auto addBindings = [&](Graphics::SPIRV_Binary* binary) {
			if (binary == nullptr) return;
			for (size_t i = 0; i < binary->BindingSetCount(); i++)
				environmentBindingSets.push_back(Graphics::ShaderResourceBindings::ShaderModuleBindingSet(&binary->BindingSet(i), binary->ShaderStages()));
		};
		addBindings(vertexShader);
		addBindings(fragmentShader);

		return Create(shaderSet, environmentBindings, environmentBindingSets.data(), environmentBindingSets.size(), device);
	}

	Reference<GraphicsEnvironment> GraphicsEnvironment::Create(
		Graphics::ShaderSet* shaderSet,
		const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& environmentBindings,
		const Graphics::ShaderResourceBindings::ShaderModuleBindingSet* environmentBindingSets, size_t environmentBindingSetCount,
		Graphics::GraphicsDevice* device) {

		std::vector<EnvironmentBinding> generatedBindings;
		if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(
			environmentBindingSets, environmentBindingSetCount, environmentBindings,
			[&](const Graphics::ShaderResourceBindings::BindingSetInfo& info) {
				if (generatedBindings.size() <= info.setIndex) generatedBindings.resize(info.setIndex + 1);
				generatedBindings[info.setIndex].binding = info.set;
			}, device->Log())) {
			device->Log()->Error("GraphicsEnvironment::Create - Failed to create environment pipeline set descriptors!");
			return nullptr;
		}
		Reference<EnvironmentPipelineSetDescriptors> environmentInstances = Object::Instantiate<EnvironmentPipelineSetDescriptors>(generatedBindings.size());
		for (size_t i = 0; i < generatedBindings.size(); i++) {
			EnvironmentBinding& binding = generatedBindings[i];
			if (binding.binding == nullptr) {
				device->Log()->Error(
					"GraphicsEnvironment::Create - Environemnt pipeline set descriptors should represent some amount of consecutive binding sets starting from set 0!");
				return nullptr;
			}
			EnvironmentPipelineSetDescriptors::DescriptorInstance* instance = &environmentInstances->instances[i];
			instance->base = binding.binding;
			binding.environmentDescriptor = instance;
		}
		Reference<GraphicsEnvironment> instance = new GraphicsEnvironment(shaderSet, std::move(generatedBindings), device);
		instance->ReleaseRef();
		return instance;
	}

	namespace {
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

		class BasicPipelineDescriptor : public virtual Graphics::PipelineDescriptor {
		private:
			const std::vector<Reference<BindingSetDescriptor>> m_bindingSets;

		public:
			inline BasicPipelineDescriptor(const std::vector<Graphics::PipelineDescriptor::BindingSetDescriptor*>& setDescriptors)
				: m_bindingSets([&]() {
				std::vector<Reference<BindingSetDescriptor>> sets(setDescriptors.size());
				for (size_t i = 0; i < setDescriptors.size(); i++) sets[i] = setDescriptors[i];
				return sets;
					}()) {}

			inline virtual size_t BindingSetCount()const override { return m_bindingSets.size(); }
			
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const { return m_bindingSets[index]; }
		};

#pragma warning(disable: 4250)
		class GraphicsPipelineDescriptor : public virtual BasicPipelineDescriptor, public virtual Graphics::GraphicsPipeline::Descriptor {
		private:
			const Reference<const GraphicsObjectDescriptor> m_sceneObject;
			const Reference<Graphics::Shader> m_vertexShader;
			const Reference<Graphics::Shader> m_fragmentShader;

		public:
			inline GraphicsPipelineDescriptor(
				const std::vector<Graphics::PipelineDescriptor::BindingSetDescriptor*>& setDescriptors, 
				const GraphicsObjectDescriptor* sceneObject, Graphics::Shader* vertexShader, Graphics::Shader* fragmentShader)
				: BasicPipelineDescriptor(setDescriptors), m_sceneObject(sceneObject), m_vertexShader(vertexShader), m_fragmentShader(fragmentShader) {}

			inline virtual Reference<Graphics::Shader> VertexShader()const override { return m_vertexShader; }
			inline virtual Reference<Graphics::Shader> FragmentShader()const override { return m_fragmentShader; }
			
			inline virtual size_t VertexBufferCount()const override { return m_sceneObject->VertexBufferCount(); }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override { return m_sceneObject->VertexBuffer(index); }
			
			inline virtual size_t InstanceBufferCount()const override { return m_sceneObject->InstanceBufferCount(); }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override { return m_sceneObject->InstanceBuffer(index); }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override { return m_sceneObject->IndexBuffer(); }

			inline virtual Graphics::IndirectDrawBufferReference IndirectBuffer() override { return m_sceneObject->IndirectBuffer(); }

			inline virtual Graphics::GraphicsPipeline::IndexType GeometryType() override { return m_sceneObject->GeometryType(); }
			inline virtual size_t IndexCount() override { return m_sceneObject->IndexCount(); }
			inline virtual size_t InstanceCount() override { return m_sceneObject->InstanceCount(); }
		};
#pragma warning(default: 4250)
	}

	Reference<Graphics::GraphicsPipeline::Descriptor> GraphicsEnvironment::CreateGraphicsPipelineDescriptor(const GraphicsObjectDescriptor* sceneObject) {
		static thread_local std::vector<Graphics::ShaderResourceBindings::BindingSetInfo> generatedBindings;
		auto cleanup = [&]() {
			generatedBindings.clear();
		};

		auto logErrorAndReturnNull = [&](const char* text) {
			m_device->Log()->Error("GraphicsEnvironment::CreateGraphicsPipeline - ", text);
			cleanup();
			return nullptr;
		};

		if (sceneObject == nullptr) return logErrorAndReturnNull("sceneObject not provided!");
		const Reference<const Graphics::ShaderClass> shader = sceneObject->ShaderClass();
		if (shader == nullptr) return logErrorAndReturnNull("sceneObject has no shader!");
		
		Reference<Graphics::SPIRV_Binary> vertexShader = m_shaderSet->GetShaderModule(shader, Graphics::PipelineStage::VERTEX);
		if (vertexShader == nullptr) return logErrorAndReturnNull("Vertex shader not found!");
		Reference<Graphics::SPIRV_Binary> fragmentShader = m_shaderSet->GetShaderModule(shader, Graphics::PipelineStage::FRAGMENT);
		if (fragmentShader == nullptr) return logErrorAndReturnNull("Fragment shader not found!");

		Reference<Graphics::Shader> vertexShaderInstance = m_shaderCache->GetShader(vertexShader);
		if (vertexShaderInstance == nullptr) logErrorAndReturnNull("Vertex shader instance could not be created!");
		Reference<Graphics::Shader> fragmentShaderInstance = m_shaderCache->GetShader(fragmentShader);
		if (fragmentShaderInstance == nullptr) logErrorAndReturnNull("Fragment shader instance could not be created!");

		{
			static thread_local std::vector<Graphics::ShaderResourceBindings::ShaderModuleBindingSet> shaderBindingSets;
			shaderBindingSets.clear();
			auto addShaderBindingSets = [&](const Graphics::SPIRV_Binary* shader, const Jimara::Graphics::PipelineStageMask stages) {
				const size_t setCount = shader->BindingSetCount();
				for (size_t i = m_environmentBindings.size(); i < setCount; i++)
					shaderBindingSets.push_back(Graphics::ShaderResourceBindings::ShaderModuleBindingSet(&shader->BindingSet(i), stages));
			};
			addShaderBindingSets(vertexShader, Graphics::StageMask(Graphics::PipelineStage::VERTEX));
			addShaderBindingSets(fragmentShader, Graphics::StageMask(Graphics::PipelineStage::FRAGMENT));
			if (!Graphics::ShaderResourceBindings::GenerateShaderBindings(
				shaderBindingSets.data(), shaderBindingSets.size(), SceneObjectResourceBindings(sceneObject, shader, m_device),
				[&](const Graphics::ShaderResourceBindings::BindingSetInfo& info) { generatedBindings.push_back(info); }, m_device->Log()))
				return logErrorAndReturnNull("Failed to generate shader binding sets for scene object!");
		}

		static thread_local std::vector<Graphics::PipelineDescriptor::BindingSetDescriptor*> setDescriptors;
		{
			setDescriptors.resize(max(vertexShader->BindingSetCount(), fragmentShader->BindingSetCount()));
			{
				size_t maxBindings = min(m_environmentBindings.size(), setDescriptors.size());
				for (size_t i = 0; i < maxBindings; i++) setDescriptors[i] = m_environmentBindings[i].environmentDescriptor;
			}
			for (size_t i = 0; i < generatedBindings.size(); i++) {
				const Graphics::ShaderResourceBindings::BindingSetInfo& setInfo = generatedBindings[i];
				setDescriptors[setInfo.setIndex] = setInfo.set;
			}
			for (size_t i = 0; i < setDescriptors.size(); i++)
				if (setDescriptors[i] == nullptr) return logErrorAndReturnNull("Incomplete set of shader binding set descriptors for the scene object!");
		}
		
		Reference<Graphics::GraphicsPipeline::Descriptor> result = 
			Object::Instantiate<GraphicsPipelineDescriptor>(setDescriptors, sceneObject, vertexShaderInstance, fragmentShaderInstance);
		cleanup();
		return result;
	}

	Graphics::PipelineDescriptor* GraphicsEnvironment::EnvironmentDescriptor()const { return m_environmentDecriptor; }


	GraphicsEnvironment::GraphicsEnvironment(Graphics::ShaderSet* shaderSet, std::vector<EnvironmentBinding>&& environmentBindings, Graphics::GraphicsDevice* device)
		: m_shaderSet(shaderSet), m_environmentBindings(std::move(environmentBindings)), m_device(device), m_shaderCache(Graphics::ShaderCache::ForDevice(device)) {
		std::vector<Graphics::PipelineDescriptor::BindingSetDescriptor*> setDescriptors(m_environmentBindings.size());
		for (size_t i = 0; i < m_environmentBindings.size(); i++) setDescriptors[i] = m_environmentBindings[i].binding;
		m_environmentDecriptor = Object::Instantiate<BasicPipelineDescriptor>(setDescriptors);
	}
}
