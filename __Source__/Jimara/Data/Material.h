#pragma once
namespace Jimara {
	class Material;
}
#include "../Graphics/Data/ShaderBinaries/ShaderClass.h"
#include <unordered_map>
#include <vector>


namespace Jimara {
	// __TODO__: Clean this crap up!

	//*
	class Material : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
	public:
		virtual Graphics::PipelineDescriptor::BindingSetDescriptor* EnvironmentDescriptor()const = 0;

		virtual Reference<Graphics::Shader> VertexShader()const = 0;

		virtual Reference<Graphics::Shader> FragmentShader()const = 0;

		inline Material() {};

	/*/
	
	/// <summary>
	/// Material, describing shader and resources, that can be applied to a rendered object
	/// Note: Material is not meant to be thread-safe; reads can be performed on multiple threads, but writes should not overlap with reads and/or other writes.
	/// </summary>
	class Material : public virtual Object {
	//*/
	public:
		/// <summary> Shader class to use </summary>
		const Graphics::ShaderClass* Shader()const;

		/// <summary>
		/// Sets shader class to use for rendering
		/// </summary>
		/// <param name="shader"> New shader to use </param>
		void SetShader(const Graphics::ShaderClass* shader);

		/// <summary>
		/// Constant buffer binding by name
		/// </summary>
		/// <param name="name"> Name of the constant buffer binding </param>
		/// <returns> Constant buffer binding if found, nullptr otherwise </returns>
		Graphics::Buffer* GetConstantBuffer(const std::string& name)const;

		/// <summary>
		/// Updates constant buffer binding
		/// </summary>
		/// <param name="name"> Name of the constant buffer binding </param>
		/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
		void SetConstantBuffer(const std::string& name, Graphics::Buffer* buffer);

		/// <summary>
		/// Structured buffer binding by name
		/// </summary>
		/// <param name="name"> Name of the structured buffer binding </param>
		/// <returns> Structured buffer binding if found, nullptr otherwise </returns>
		Graphics::ArrayBuffer* GetStructuredBuffer(const std::string& name)const;

		/// <summary>
		/// Updates structured buffer binding
		/// </summary>
		/// <param name="name"> Name of the structured buffer binding </param>
		/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
		void SetStructuredBuffer(const std::string& name, Graphics::ArrayBuffer* buffer);

		/// <summary>
		/// Texture sampler binding by name
		/// </summary>
		/// <param name="name"> Name of the texture sampler binding </param>
		/// <returns> Texture sampler binding if found, nullptr otherwise </returns>
		Graphics::TextureSampler* GetTextureSampler(const std::string& name)const;

		/// <summary>
		/// Updates texture sampler binding
		/// </summary>
		/// <param name="name"> Name of the texture sampler binding </param>
		/// <param name="buffer"> New sampler to set (nullptr removes the binding) </param>
		void SetTextureSampler(const std::string& name, Graphics::TextureSampler* sampler);

		/// <summary>
		/// Cached instance of a material (Update() call is requred to update binding values)
		/// </summary>
		class CachedInstance;

		/// <summary>
		/// Material instance (this one, basically, has a fixed set of the shader class and the available resource bindings)
		/// Note: Shader bindings are automatically updated for Instance, when underluying material changes their values, 
		/// but new resources are not added or removed dynamically. CachedInstance inherits Instance, but it needs Update() call to be up to date with the material.
		/// </summary>
		class Instance : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="material"> Material, to 'instantiate' </param>
			Instance(const Material* material);

			/// <summary> Shader class used by the Instance </summary>
			const Graphics::ShaderClass* Shader()const;

			/// <summary> Number of available constant buffer bindings </summary>
			size_t ConstantBufferCount()const;

			/// <summary>
			/// Constant buffer binding name by index
			/// </summary>
			/// <param name="index"> Constant buffer index </param>
			/// <returns> Constant buffer binding name </returns>
			const std::string& ConstantBufferName(size_t index)const;

			/// <summary>
			/// Constant buffer binding by index
			/// </summary>
			/// <param name="index"> Constant buffer index </param>
			/// <returns> Constant buffer binding </returns>
			const Graphics::ShaderResourceBindings::ConstantBufferBinding* ConstantBuffer(size_t index)const;

			/// <summary> Number of available structured buffer bindings </summary>
			size_t StructuredBufferCount()const;

			/// <summary>
			/// Structured buffer binding name by index
			/// </summary>
			/// <param name="index"> Structured buffer index </param>
			/// <returns> Structured buffer binding name </returns>
			const std::string& StructuredBufferName(size_t index)const;

			/// <summary>
			/// Structured buffer binding by index
			/// </summary>
			/// <param name="index"> Structured buffer index </param>
			/// <returns> Structured buffer binding </returns>
			const Graphics::ShaderResourceBindings::StructuredBufferBinding* StructuredBuffer(size_t index)const;

			/// <summary> Number of available texture sampler bindings </summary>
			size_t TextureSamplerCount()const;
			
			/// <summary>
			/// Texture sampler binding name by index
			/// </summary>
			/// <param name="index"> Texture sampler buffer index </param>
			/// <returns> Texture sampler buffer binding name </returns>
			const std::string& TextureSamplerName(size_t index)const;

			/// <summary>
			/// Texture sampler binding by index
			/// </summary>
			/// <param name="index"> Texture sampler buffer index </param>
			/// <returns> Texture sampler buffer binding </returns>
			const Graphics::ShaderResourceBindings::TextureSamplerBinding* TextureSampler(size_t index)const;

		private:
			// Shader class used by the Instance
			Reference<const Graphics::ShaderClass> m_shader;

			// Constant buffer bindings
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::ConstantBufferBinding>>> m_constantBuffers;

			// Structured buffer bindings
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding>>> m_structuredBuffers;

			// Texture sampler bindings
			std::vector<std::pair<const std::string*, Reference<Graphics::ShaderResourceBindings::TextureSamplerBinding>>> m_textureSamplers;

			// CachedInstance has to access it's own internals
			friend class CachedInstance;
		};

		/// <summary>
		/// Cached instance of a material (Update() call is requred to update binding values)
		/// </summary>
		class CachedInstance : public virtual Instance {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="base"> Instance to cache </param>
			CachedInstance(const Instance* base);

			/// <summary> Updates binding </summary>
			void Update();

		private:
			// Instance to cache
			const Reference<const Instance> m_base;
		};

		/// <summary>
		/// Shared instance of the material 
		/// (always up to date with the bindings and shader class; will change automatically, when the old one becomes incomp[atible with current state)
		/// </summary>
		const Instance* SharedInstance()const;


	private:
		// Shader class used by the Material
		Reference<const Graphics::ShaderClass> m_shaderClass;

		// Constant buffer bindings
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedConstantBufferBinding>> m_constantBuffers;

		// Structured buffer bindings
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>> m_structuredBuffers;

		// Texture sampler bindings
		std::unordered_map<std::string_view, Reference<Graphics::ShaderResourceBindings::NamedTextureSamplerBinding>> m_textureSamplers;

		// True, when the shared instance becomes invalid
		mutable std::atomic<bool> m_dirty = true;

		// Shared instance
		mutable Reference<const Instance> m_sharedInstance;

		// Lock for preventing multi-initialisation of m_sharedInstance
		std::mutex mutable m_instanceLock;
	};
}
