#include "Material.h"


namespace Jimara {
	const Graphics::ShaderClass* Material::Shader()const { return m_shaderClass; }
	void Material::SetShader(const Graphics::ShaderClass* shader) {
		if (m_shaderClass == shader) return;
		m_shaderClass = shader;
		m_dirty = true;
	}

	namespace {
		template<typename ResourceType>
		inline static ResourceType* Find(
			const std::string& name,
			const std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>& index) {
			typename std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>::const_iterator it = index.find(name);
			if (it == index.end()) return nullptr;
			else return it->second->BoundObject();
		}

		template<typename ResourceType>
		inline static bool Replace(
			const std::string& name, ResourceType* newValue,
			std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>& index) {
			typename std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>::iterator it = index.find(name);
			if (it == index.end()) {
				if (newValue == nullptr) return false;
				Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>> binding =
					Object::Instantiate<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>(name, newValue);
				index.insert(std::make_pair(binding->BindingName(), binding));
				return true;
			}
			else if (newValue == nullptr) {
				index.erase(it);
				return true;
			}
			else {
				it->second->BoundObject() = newValue;
				return false;
			}
		}
	}

	Graphics::Buffer* Material::GetConstantBuffer(const std::string& name)const {
		return Find(name, m_constantBuffers);
	}

	void Material::SetConstantBuffer(const std::string& name, Graphics::Buffer* buffer) {
		m_dirty = Replace(name, buffer, m_constantBuffers);
	}

	Graphics::ArrayBuffer* Material::GetStructuredBuffer(const std::string& name)const {
		return Find(name, m_structuredBuffers);
	}
	void Material::SetStructuredBuffer(const std::string& name, Graphics::ArrayBuffer* buffer) {
		m_dirty = Replace(name, buffer, m_structuredBuffers);
	}

	Graphics::TextureSampler* Material::GetTextureSampler(const std::string& name)const {
		return Find(name, m_textureSamplers);
	}
	void Material::SetTextureSampler(const std::string& name, Graphics::TextureSampler* sampler) {
		m_dirty = Replace(name, sampler, m_textureSamplers);
	}


	namespace {
		template<typename ResourceType>
		void CollectResourceReferences(
			const std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>& source,
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>>& destination) {
			typedef typename std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>::const_iterator Iterator;
			for (Iterator it = source.begin(); it != source.end(); ++it) {
				Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>* resource = it->second;
				if (resource == nullptr) continue;
				destination.push_back(std::make_pair(&(resource->BindingName()), resource));
			}
		}
	}

	Material::Instance::Instance(const Material* material) {
		if (material == nullptr) return;
		m_shader = material->m_shaderClass;
		CollectResourceReferences(material->m_constantBuffers, m_constantBuffers);
		CollectResourceReferences(material->m_structuredBuffers, m_structuredBuffers);
		CollectResourceReferences(material->m_textureSamplers, m_textureSamplers);
	}

	const Graphics::ShaderClass* Material::Instance::Shader()const { return m_shader; }

	size_t Material::Instance::ConstantBufferCount()const { return m_constantBuffers.size(); }
	const std::string& Material::Instance::ConstantBufferName(size_t index)const { return *m_constantBuffers[index].first; }
	const Graphics::ShaderResourceBindings::ConstantBufferBinding* Material::Instance::ConstantBuffer(size_t index)const { return m_constantBuffers[index].second; }

	size_t Material::Instance::StructuredBufferCount()const { return m_structuredBuffers.size(); }
	const std::string& Material::Instance::StructuredBufferName(size_t index)const { return *m_structuredBuffers[index].first; }
	const Graphics::ShaderResourceBindings::StructuredBufferBinding* Material::Instance::StructuredBuffer(size_t index)const { return m_structuredBuffers[index].second; }

	size_t Material::Instance::TextureSamplerCount()const { return m_textureSamplers.size(); }
	const std::string& Material::Instance::TextureSamplerName(size_t index)const { return *m_textureSamplers[index].first; }
	const Graphics::ShaderResourceBindings::TextureSamplerBinding* Material::Instance::TextureSampler(size_t index)const { return m_textureSamplers[index].second; }


	namespace {
		template<typename ResourceType>
		class ResourceBindingGroup : public virtual Object {
		public:
			class Binding : public virtual Graphics::ShaderResourceBindings::ShaderBinding<ResourceType> {
			public:
				mutable Reference<ResourceBindingGroup> group;

			protected:
				inline virtual void OnOutOfScope()const override { group = nullptr; }
			};

			std::vector<Binding> group;

			inline ResourceBindingGroup(size_t count) : group(count) {
				for (size_t i = 0; i < count; i++) {
					Binding& binding = group[i];
					binding.ReleaseRef();
					binding.group = this;
				}
			}
		};


		template<typename ResourceType>
		inline void CopyBoundResources(
			const std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>>& src,
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>>& dst) {
			for (size_t i = 0; i < src.size(); i++) dst[i].second->BoundObject() = src[i].second->BoundObject();
		}

		template<typename ResourceType>
		inline void MakeMirror(
			const std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>>& src,
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>>& dst) {
			Reference<ResourceBindingGroup<ResourceType>> group = Object::Instantiate<ResourceBindingGroup<ResourceType>>(src.size());
			dst.resize(src.size());
			for (size_t i = 0; i < src.size(); i++) {
				std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ShaderBinding<ResourceType>>>& destination = dst[i];
				destination.first = src[i].first;
				destination.second = (group->group.data() + i);
			}
			CopyBoundResources(src, dst);
		}
	}

	Material::CachedInstance::CachedInstance(const Instance* base) : Instance(nullptr), m_base(base) {
		m_shader = m_base->m_shader;
		MakeMirror(m_base->m_constantBuffers, m_constantBuffers);
		MakeMirror(m_base->m_structuredBuffers, m_structuredBuffers);
		MakeMirror(m_base->m_textureSamplers, m_textureSamplers);
	}

	void Material::CachedInstance::Update() {
		CopyBoundResources(m_base->m_constantBuffers, m_constantBuffers);
		CopyBoundResources(m_base->m_structuredBuffers, m_structuredBuffers);
		CopyBoundResources(m_base->m_textureSamplers, m_textureSamplers);
	}

	const Material::Instance* Material::SharedInstance()const {
		if (m_dirty) {
			std::unique_lock<std::mutex> lock(m_instanceLock);
			m_dirty = false;
			m_sharedInstance = Object::Instantiate<Instance>(this);
		}
		return m_sharedInstance;
	}
}
