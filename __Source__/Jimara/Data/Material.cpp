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
}
