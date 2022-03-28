#include "Material.h"


namespace Jimara {
	Event<const Material*>& Material::OnMaterialDirty()const { return m_onMaterialDirty; }

	Event<const Material*>& Material::OnInvalidateSharedInstance()const { return m_onMaterialDirty; }

	namespace {
		template<typename ResourceType>
		inline static ResourceType* Find(
			const std::string_view& name,
			const std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>& index) {
			typename std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>::const_iterator it = index.find(name);
			if (it == index.end()) return nullptr;
			else return it->second->BoundObject();
		}

		template<typename ResourceType>
		inline static void Replace(
			const std::string_view& name, ResourceType* newValue,
			std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>& index,
			bool& dirty, bool& invalidateSharedInstance) {
			typename std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>>::iterator it = index.find(name);
			if (it == index.end()) {
				if (newValue == nullptr) return;
				Reference<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>> binding =
					Object::Instantiate<Graphics::ShaderResourceBindings::NamedShaderBinding<ResourceType>>(name, newValue);
				index.insert(std::make_pair(binding->BindingName(), binding));
				dirty = true;
				invalidateSharedInstance = true;
			}
			else if (newValue == nullptr) {
				index.erase(it);
				dirty = true;
				invalidateSharedInstance = true;
			}
			else if (it->second->BoundObject() != newValue) {
				it->second->BoundObject() = newValue;
				dirty = true;
			}
		}
	}



	Material::Reader::Reader(const Material& material) : Reader(&material) {}

	Material::Reader::Reader(const Material* material) : m_material(material), m_lock(material->m_readWriteLock) {}

	const Graphics::ShaderClass* Material::Reader::Shader()const { return m_material->m_shaderClass; }

	Graphics::Buffer* Material::Reader::GetConstantBuffer(const std::string_view& name)const {
		return Find(name, m_material->m_constantBuffers);
	}

	Graphics::ArrayBuffer* Material::Reader::GetStructuredBuffer(const std::string_view& name)const {
		return Find(name, m_material->m_structuredBuffers);
	}

	Graphics::TextureSampler* Material::Reader::GetTextureSampler(const std::string_view& name)const {
		return Find(name, m_material->m_textureSamplers);
	}

	const Reference<const Material::Instance> Material::Reader::SharedInstance()const {
		Reference<const Instance> instance = m_material->m_sharedInstance;
		if (instance == nullptr) {
			std::unique_lock<std::mutex> lock(m_material->m_instanceLock);
			m_material->m_sharedInstance = Object::Instantiate<Instance>(this);
			instance = m_material->m_sharedInstance;
		}
		return instance;
	}



	Material::Writer::Writer(Material& material) : Writer(&material) {}

	Material::Writer::Writer(Material* material)
		: m_material(material), m_dirty(false), m_invalidateSharedInstance(false) {
		m_material->m_readWriteLock.lock();
	}

	Material::Writer::~Writer() {
		if (m_invalidateSharedInstance) m_material->m_sharedInstance = nullptr;
		m_material->m_readWriteLock.unlock();
		if (m_dirty) m_material->m_onMaterialDirty(m_material);
		if (m_invalidateSharedInstance) m_material->m_onInvalidateSharedInstance(m_material);
	}

	const Graphics::ShaderClass* Material::Writer::Shader()const { return m_material->m_shaderClass; }
	void Material::Writer::SetShader(const Graphics::ShaderClass* shader) {
		if (m_material->m_shaderClass == shader) return;
		m_material->m_shaderClass = shader;
		m_dirty = true;
		m_invalidateSharedInstance = true;
	}

	Graphics::Buffer* Material::Writer::GetConstantBuffer(const std::string_view& name)const {
		return Find(name, m_material->m_constantBuffers);
	}

	void Material::Writer::SetConstantBuffer(const std::string_view& name, Graphics::Buffer* buffer) {
		Replace(name, buffer, m_material->m_constantBuffers, m_dirty, m_invalidateSharedInstance);
	}

	Graphics::ArrayBuffer* Material::Writer::GetStructuredBuffer(const std::string_view& name)const {
		return Find(name, m_material->m_structuredBuffers);
	}
	void Material::Writer::SetStructuredBuffer(const std::string_view& name, Graphics::ArrayBuffer* buffer) {
		Replace(name, buffer, m_material->m_structuredBuffers, m_dirty, m_invalidateSharedInstance);
	}

	Graphics::TextureSampler* Material::Writer::GetTextureSampler(const std::string_view& name)const {
		return Find(name, m_material->m_textureSamplers);
	}
	void Material::Writer::SetTextureSampler(const std::string_view& name, Graphics::TextureSampler* sampler) {
		Replace(name, sampler, m_material->m_textureSamplers, m_dirty, m_invalidateSharedInstance);
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

	Material::Instance::Instance(const Reader* reader) {
		if (reader == nullptr) return;
		const Material* material = reader->m_material;
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




	Material::Serializer::Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
		: ItemSerializer(name, hint, attributes) {}

	const Material::Serializer* Material::Serializer::Instance() {
		static const Serializer instance;
		return &instance;
	}

	void Material::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Material* target)const {
		if (target == nullptr) return;

		Reference<const Graphics::ShaderClass::Set> shaderClasses = Graphics::ShaderClass::Set::All();
		assert(shaderClasses != nullptr);

		Writer writer(target);

		{
			const Reference<const Graphics::ShaderClass> initialShaderClass = writer.Shader();
			const Graphics::ShaderClass* shaderClassPtr = initialShaderClass;
			recordElement(shaderClasses->ShaderClassSelector()->Serialize(shaderClassPtr));
			if (shaderClassPtr != initialShaderClass) writer.SetShader(shaderClassPtr);
		}

		if (writer.Shader() != nullptr)
			writer.Shader()->SerializeBindings(recordElement, &writer);
	}
}
