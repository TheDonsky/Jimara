#pragma once
namespace Jimara {
	class Material;
	class CachedMaterial;
}
#include "../Graphics/GraphicsDevice.h"
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
		template<typename ResourceType>
		class ResourceBinding : public virtual Object {
		private:
			const std::string m_bindingName;
			Reference<ResourceType> m_object;

		public:
			ResourceBinding(std::string&& bindingName, ResourceType* object) : m_bindingName(std::move(bindingName)), m_object(object) {}

			ResourceBinding(const std::string& bindingName, ResourceType* object) : ResourceBinding(std::move(std::string(bindingName)), object) {}

			const std::string& BindingName()const { return m_bindingName; }

			Reference<ResourceType>& Object() { return m_object; }

			ResourceType* Object()const { return m_object; }
		};

		typedef ResourceBinding<Graphics::Buffer> ConstantBufferBinding;

		typedef ResourceBinding<Graphics::ArrayBuffer> StructuredBufferBinding;

		typedef ResourceBinding<Graphics::TextureSampler> TextureSamplerBinding;

		typedef std::string ShaderIdentifier;

		class MaterialDescriptor {
		public:
			virtual const ShaderIdentifier& ShaderId()const = 0;

			virtual size_t ConstantBufferCount()const = 0;

			virtual Reference<ConstantBufferBinding> ConstantBuffer(size_t index)const = 0;

			virtual size_t StructuredBufferCount()const = 0;

			virtual Reference<StructuredBufferBinding> StructuredBuffer(size_t index)const = 0;

			virtual size_t TextureSamplerCount()const = 0;

			virtual Reference<TextureSamplerBinding> TextureSampler(size_t index)const = 0;
		};

		struct MaterialBuilder : public MaterialDescriptor {
			template<typename ResourceType>
			struct ResourceBindingDesc {
				const char* bindingName;
				ResourceType* resource;
			};

			typedef ResourceBindingDesc<Graphics::Buffer> ConstantBufferBindDesc;

			typedef ResourceBindingDesc<Graphics::ArrayBuffer> StructuredBufferBindDesc;

			typedef ResourceBindingDesc<Graphics::TextureSampler> TextureSamplerBindDesc;


			const ShaderIdentifier* shaderId = nullptr;

			size_t constantBufferCount = 0;

			ConstantBufferBindDesc* constantBuffers = nullptr;


			size_t structuredBufferCount = 0;

			StructuredBufferBindDesc* structuredBuffers = nullptr;


			size_t textureSamplerCount = 0;

			TextureSamplerBindDesc* textureSamplers = nullptr;



			virtual const ShaderIdentifier& ShaderId()const override;

			virtual size_t ConstantBufferCount()const override;

			virtual Reference<ConstantBufferBinding> ConstantBuffer(size_t index)const override;

			virtual size_t StructuredBufferCount()const override;

			virtual Reference<StructuredBufferBinding> StructuredBuffer(size_t index)const override;

			virtual size_t TextureSamplerCount()const override;

			virtual Reference<TextureSamplerBinding> TextureSampler(size_t index)const override;
		};


		Material(const MaterialDescriptor& descriptor);

		virtual ~Material();

		const ShaderIdentifier& ShaderId()const;

		size_t ConstantBufferBindingCount()const;

		ConstantBufferBinding* GetConstantBufferBinding(size_t index);

		const ConstantBufferBinding* GetConstantBufferBinding(size_t index)const;

		ConstantBufferBinding* FindConstantBufferBinding(const std::string& bindingName);

		const ConstantBufferBinding* FindConstantBufferBinding(const std::string& bindingName)const;

		size_t StructuredBufferBindingCount()const;

		StructuredBufferBinding* GetStructuredBufferBinding(size_t index);

		const StructuredBufferBinding* GetStructuredBufferBinding(size_t index)const;

		StructuredBufferBinding* FindStructuredBufferBinding(const std::string& bindingName);

		const StructuredBufferBinding* FindStructuredBufferBinding(const std::string& bindingName)const;

		size_t TextureSamplerBindingCount()const;

		TextureSamplerBinding* GetTextureSamplerBinding(size_t index);

		const TextureSamplerBinding* GetTextureSamplerBinding(size_t index)const;

		TextureSamplerBinding* FindTextureSamplerBinding(const std::string& bindingName);

		const TextureSamplerBinding* FindTextureSamplerBinding(const std::string& bindingName)const;


	private:
		template<typename Value>
		class BindingMap {
		private:
			std::unordered_map<std::string, size_t> m_index;
			std::vector<Reference<Value>> m_bindings;

		public:
			inline void Add(Reference<Value>&& value) {
				m_index[value->BindingName()] = m_bindings.size();
				m_bindings.push_back(std::move(value));
			}

			inline size_t Size()const { return m_bindings.size(); }

			inline Value* operator[](const std::string& name)const {
				std::unordered_map<std::string, size_t>::const_iterator it = m_index.find(name);
				if (it == m_index.end()) return nullptr;
				else return m_bindings[it->second];
			}

			inline Value* operator[](size_t index)const { return m_bindings[index]; }
		};

		ShaderIdentifier m_shaderId;

		BindingMap<ConstantBufferBinding> m_constantBuffers;

		BindingMap<StructuredBufferBinding> m_structuredBuffers;

		BindingMap<TextureSamplerBinding> m_textureSamplers;
	};


	class CachedMaterial : public virtual Material {
	public:
		CachedMaterial(const Material* baseMaterial);

		void Update();

	private:
		Reference<const Material> m_baseMaterial;
	};
}
