#include "Material.h"


namespace Jimara {
	const Material::ShaderIdentifier& Material::MaterialBuilder::ShaderId()const { return *shaderId; }

	size_t Material::MaterialBuilder::ConstantBufferCount()const { return constantBufferCount; }

	Reference<Material::ConstantBufferBinding> Material::MaterialBuilder::ConstantBuffer(size_t index)const {
		const ConstantBufferBindDesc& desc = constantBuffers[index];
		return Object::Instantiate<ConstantBufferBinding>(std::move(std::string(desc.bindingName)), desc.resource);
	}

	size_t Material::MaterialBuilder::StructuredBufferCount()const { return structuredBufferCount; }

	Reference<Material::StructuredBufferBinding> Material::MaterialBuilder::StructuredBuffer(size_t index)const {
		const StructuredBufferBindDesc& desc = structuredBuffers[index];
		return Object::Instantiate<StructuredBufferBinding>(std::move(std::string(desc.bindingName)), desc.resource);
	}

	size_t Material::MaterialBuilder::TextureSamplerCount()const { return textureSamplerCount; }

	Reference<Material::TextureSamplerBinding> Material::MaterialBuilder::TextureSampler(size_t index)const {
		const TextureSamplerBindDesc& desc = textureSamplers[index];
		return Object::Instantiate<TextureSamplerBinding>(std::move(std::string(desc.bindingName)), desc.resource);
	}

	Material::Material(const MaterialDescriptor& descriptor) {
		m_shaderId = descriptor.ShaderId();
		m_shaderIdRevision = 0;
		
		const size_t constantBufferCount = descriptor.ConstantBufferCount();
		for (size_t i = 0; i < constantBufferCount; i++)
			m_constantBuffers.Add(descriptor.ConstantBuffer(i));

		const size_t structuredBufferCount = descriptor.StructuredBufferCount();
		for (size_t i = 0; i < structuredBufferCount; i++)
			m_structuredBuffers.Add(descriptor.StructuredBuffer(i));

		const size_t textureSamplerCount = descriptor.TextureSamplerCount();
		for (size_t i = 0; i < textureSamplerCount; i++)
			m_textureSamplers.Add(descriptor.TextureSampler(i));
	}

	Material::~Material() {}

	const Material::ShaderIdentifier& Material::ShaderId()const { return m_shaderId; }

	void Material::SetShaderIdentifier(const ShaderIdentifier& newIdentifier) { m_shaderId = newIdentifier; m_shaderIdRevision++; }

	size_t Material::ConstantBufferBindingCount()const { return m_constantBuffers.Size(); }

	Material::ConstantBufferBinding* Material::GetConstantBufferBinding(size_t index) { return m_constantBuffers[index]; }

	const Material::ConstantBufferBinding* Material::GetConstantBufferBinding(size_t index)const { return m_constantBuffers[index]; }

	Material::ConstantBufferBinding* Material::FindConstantBufferBinding(const std::string& bindingName) { return m_constantBuffers[bindingName]; }

	const Material::ConstantBufferBinding* Material::FindConstantBufferBinding(const std::string& bindingName)const { return m_constantBuffers[bindingName]; }

	size_t Material::StructuredBufferBindingCount()const { return m_structuredBuffers.Size(); }

	Material::StructuredBufferBinding* Material::GetStructuredBufferBinding(size_t index) { return m_structuredBuffers[index]; }

	const Material::StructuredBufferBinding* Material::GetStructuredBufferBinding(size_t index)const { return m_structuredBuffers[index]; }

	Material::StructuredBufferBinding* Material::FindStructuredBufferBinding(const std::string& bindingName) { return m_structuredBuffers[bindingName]; }

	const Material::StructuredBufferBinding* Material::FindStructuredBufferBinding(const std::string& bindingName)const { return m_structuredBuffers[bindingName]; }

	size_t Material::TextureSamplerBindingCount()const { return m_textureSamplers.Size(); }

	Material::TextureSamplerBinding* Material::GetTextureSamplerBinding(size_t index) { return m_textureSamplers[index]; }

	const Material::TextureSamplerBinding* Material::GetTextureSamplerBinding(size_t index)const { return m_textureSamplers[index]; }

	Material::TextureSamplerBinding* Material::FindTextureSamplerBinding(const std::string& bindingName) { return m_textureSamplers[bindingName]; }

	const Material::TextureSamplerBinding* Material::FindTextureSamplerBinding(const std::string& bindingName)const { return m_textureSamplers[bindingName]; }





	namespace {
		class CachedMaterialBuilder : public virtual Material::MaterialDescriptor {
		private:
			const Material* m_base;

		public:
			CachedMaterialBuilder(const Material* baseMaterial) : m_base(baseMaterial) {}

			virtual const Material::ShaderIdentifier& ShaderId()const override { return m_base->ShaderId(); }

			virtual size_t ConstantBufferCount()const override { return m_base->ConstantBufferBindingCount(); }

			virtual Reference<Material::ConstantBufferBinding> ConstantBuffer(size_t index)const override {
				return Object::Instantiate<Material::ConstantBufferBinding>(m_base->GetConstantBufferBinding(index)->BindingName(), nullptr);
			}

			virtual size_t StructuredBufferCount()const override { return m_base->StructuredBufferBindingCount(); }

			virtual Reference<Material::StructuredBufferBinding> StructuredBuffer(size_t index)const override {
				return Object::Instantiate<Material::StructuredBufferBinding>(m_base->GetStructuredBufferBinding(index)->BindingName(), nullptr);
			}

			virtual size_t TextureSamplerCount()const override { return m_base->TextureSamplerBindingCount(); }

			virtual Reference<Material::TextureSamplerBinding> TextureSampler(size_t index)const override {
				return Object::Instantiate<Material::TextureSamplerBinding>(m_base->GetTextureSamplerBinding(index)->BindingName(), nullptr);
			}
		};
	}

	CachedMaterial::CachedMaterial(const Material* baseMaterial) 
		: Material(CachedMaterialBuilder(baseMaterial)), m_baseMaterial(baseMaterial) {
		m_lastShaderIdRevision = m_baseMaterial->m_shaderIdRevision.load();
		m_shaderIdRevision = m_lastShaderIdRevision.load();
		Update();
	}

	void CachedMaterial::Update() {
		if (m_lastShaderIdRevision != m_baseMaterial->m_shaderIdRevision || m_shaderIdRevision != m_lastShaderIdRevision) {
			SetShaderIdentifier(m_baseMaterial->ShaderId());
			m_lastShaderIdRevision = m_baseMaterial->m_shaderIdRevision.load();
			m_shaderIdRevision = m_lastShaderIdRevision.load();
		}

		const size_t constantBufferCount = ConstantBufferBindingCount();
		for (size_t i = 0; i < constantBufferCount; i++)
			GetConstantBufferBinding(i)->Object() = m_baseMaterial->GetConstantBufferBinding(i)->Object();

		const size_t structuredBufferCount = StructuredBufferBindingCount();
		for (size_t i = 0; i < structuredBufferCount; i++)
			GetStructuredBufferBinding(i)->Object() = m_baseMaterial->GetStructuredBufferBinding(i)->Object();

		const size_t textureSamplerCount = TextureSamplerBindingCount();
		for (size_t i = 0; i < constantBufferCount; i++)
			GetTextureSamplerBinding(i)->Object() = m_baseMaterial->GetTextureSamplerBinding(i)->Object();
	}
}
