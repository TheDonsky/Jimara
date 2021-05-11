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

		class CachedInstance;

		class Instance : public virtual Object {
		public:
			Instance(const Material* material);

			const Graphics::ShaderClass* Shader()const;

			size_t ConstantBufferCount()const;
			const std::string& ConstantBufferName(size_t index)const;
			const Graphics::ShaderResourceBindings::ConstantBufferBinding* ConstantBuffer(size_t index)const;

			size_t StructuredBufferCount()const;
			const std::string& StructuredBufferName(size_t index)const;
			const Graphics::ShaderResourceBindings::StructuredBufferBinding* StructuredBuffer(size_t index)const;

			size_t TextureSamplerCount()const;
			const std::string& TextureSamplerName(size_t index)const;
			const Graphics::ShaderResourceBindings::TextureSamplerBinding* TextureSampler(size_t index)const;

		private:
			Reference<const Graphics::ShaderClass> m_shader;
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding>>> m_constantBuffers;
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding>>> m_structuredBuffers;
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::TextureSamplerBinding>>> m_textureSamplers;

			friend class CachedInstance;
		};

		class CachedInstance : public virtual Instance {
		public:
			CachedInstance(const Instance* base);

			void Update();

		private:
			const Reference<const Instance> m_base;
		};

		const Instance* SharedInstance()const;

	private:
		Reference<const Graphics::ShaderClass> m_shaderClass;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>> m_constantBuffers;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>> m_structuredBuffers;
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedTextureSamplerBinding>> m_textureSamplers;
		mutable std::atomic<bool> m_dirty = true;
		mutable Reference<const Instance> m_sharedInstance;
		std::mutex mutable m_instanceLock;
	};
}
