#pragma once
namespace Jimara {
	class Material;
	class CachedMaterial;
}
#include "../Graphics/Data/ShaderBinaries/ShaderClass.h"
#include <unordered_map>
#include <vector>


namespace Jimara {
	//*
	class Material : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
	public:
		virtual Graphics::PipelineDescriptor::BindingSetDescriptor* EnvironmentDescriptor()const = 0;

		virtual Reference<Graphics::Shader> VertexShader()const = 0;

		virtual Reference<Graphics::Shader> FragmentShader()const = 0;

		inline Material() {};

	/*/
	class Material : public virtual Object {
	//*/
	public:
		const Graphics::ShaderClass* Shader()const;
		void SetShader(const Graphics::ShaderClass* shader);

		Graphics::Buffer* GetConstantBuffer(const std::string& name)const;
		void SetConstantBuffer(const std::string& name, Graphics::Buffer* buffer);

		Graphics::ArrayBuffer* GetStructuredBuffer(const std::string& name)const;
		void SetStructuredBuffer(const std::string& name, Graphics::ArrayBuffer* buffer);

		Graphics::TextureSampler* GetTextureSampler(const std::string& name)const;
		void SetTextureSampler(const std::string& name, Graphics::TextureSampler* sampler);


	private:
		Reference<const Graphics::ShaderClass> m_shaderClass;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>> m_constantBuffers;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>> m_structuredBuffers;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedTextureSamplerBinding>> m_textureSamplers;
		std::atomic<bool> m_dirty = true;
	};
}
