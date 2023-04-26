#pragma once
namespace Jimara {
	class Material;
}
#include "../Graphics/Data/ShaderBinaries/ShaderClass.h"
#include "AssetDatabase/AssetDatabase.h"
#include <unordered_map>
#include <vector>
#include <shared_mutex>


namespace Jimara {
	/// <summary>
	/// Material, describing shader and resources, that can be applied to a rendered object
	/// Note: Material is not meant to be thread-safe; reads can be performed on multiple threads, but writes should not overlap with reads and/or other writes.
	/// </summary>
	class JIMARA_API Material : public virtual Resource {
	public:
		class Reader;
		class Writer;
		class Instance;
		class CachedInstance;

		/// <summary>
		/// Type definition for resource binding
		/// </summary>
		/// <typeparam name="ResourceType"> Graphics resource type </typeparam>
		template<typename ResourceType> using ResourceBinding = Graphics::ResourceBinding<ResourceType>;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device, this material is tied to </param>
		inline Material(Graphics::GraphicsDevice* graphicsDevice) : m_graphicsDevice(graphicsDevice) {};

		/// <summary> Graphics device, this material is tied to </summary>
		inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

		/// <summary> Invoked, whenever any of the material properties gets altered </summary>
		Event<const Material*>& OnMaterialDirty()const;

		/// <summary> Invoked, whenever the shared instance gets invalidated </summary>
		Event<const Material*>& OnInvalidateSharedInstance()const;

		/// <summary>
		/// Material reader (multiple readers can exist at once)
		/// </summary>
		class JIMARA_API Reader {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="material"> Material </param>
			Reader(const Material& material);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="material"> Material </param>
			Reader(const Material* material);

			/// <summary> Shader class to use </summary>
			const Graphics::ShaderClass* Shader()const;

			/// <summary>
			/// Constant buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <returns> Constant buffer binding if found, nullptr otherwise </returns>
			Graphics::Buffer* GetConstantBuffer(const std::string_view& name)const;

			/// <summary>
			/// Structured buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <returns> Structured buffer binding if found, nullptr otherwise </returns>
			Graphics::ArrayBuffer* GetStructuredBuffer(const std::string_view& name)const;

			/// <summary>
			/// Texture sampler binding by name
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <returns> Texture sampler binding if found, nullptr otherwise </returns>
			Graphics::TextureSampler* GetTextureSampler(const std::string_view& name)const;

			/// <summary>
			/// Shared instance of the material 
			/// (always up to date with the bindings and shader class; will change automatically, when the old one becomes incomp[atible with current state)
			/// </summary>
			const Reference<const Instance> SharedInstance()const;


		private:
			// Target
			Reference<const Material> m_material;

			// Lock
			std::shared_lock<std::shared_mutex> m_lock;

			// Instance needs access to material
			friend class Instance;
		};

		/// <summary>
		/// Material writer (only one can exist at a time)
		/// </summary>
		class JIMARA_API Writer : public virtual Graphics::ShaderClass::Bindings {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="material"> Material </param>
			Writer(Material& material);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="material"> Material </param>
			Writer(Material* material);

			/// <summary> Virtual destructor </summary>
			virtual ~Writer();

			/// <summary> Shader class to use </summary>
			const Graphics::ShaderClass* Shader()const;

			/// <summary>
			/// Sets shader class to use for rendering
			/// </summary>
			/// <param name="shader"> New shader to use </param>
			void SetShader(const Graphics::ShaderClass* shader);

			/// <summary> Graphics device, target material is tied to </summary>
			virtual inline Graphics::GraphicsDevice* GraphicsDevice()const final override { return m_material->GraphicsDevice(); }

			/// <summary>
			/// Constant buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <returns> Constant buffer binding if found, nullptr otherwise </returns>
			virtual Graphics::Buffer* GetConstantBuffer(const std::string_view& name)const override;

			/// <summary>
			/// Updates constant buffer binding
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetConstantBuffer(const std::string_view& name, Graphics::Buffer* buffer) override;

			/// <summary>
			/// Structured buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <returns> Structured buffer binding if found, nullptr otherwise </returns>
			virtual Graphics::ArrayBuffer* GetStructuredBuffer(const std::string_view& name)const override;

			/// <summary>
			/// Updates structured buffer binding
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetStructuredBuffer(const std::string_view& name, Graphics::ArrayBuffer* buffer) override;

			/// <summary>
			/// Texture sampler binding by name
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <returns> Texture sampler binding if found, nullptr otherwise </returns>
			virtual Graphics::TextureSampler* GetTextureSampler(const std::string_view& name)const override;

			/// <summary>
			/// Updates texture sampler binding
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <param name="buffer"> New sampler to set (nullptr removes the binding) </param>
			virtual void SetTextureSampler(const std::string_view& name, Graphics::TextureSampler* sampler) override;


		private:
			// Target
			Reference<Material> m_material;

			// 'Dirty' flag
			bool m_dirty;

			// Flag, that tells that the shared instance is no longer valid
			bool m_invalidateSharedInstance;
		};

		/// <summary>
		/// Material instance (this one, basically, has a fixed set of the shader class and the available resource bindings)
		/// Note: Shader bindings are automatically updated for Instance, when underluying material changes their values, 
		/// but new resources are not added or removed dynamically. CachedInstance inherits Instance, but it needs Update() call to be up to date with the material.
		/// </summary>
		class JIMARA_API Instance : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="reader"> Material reader </param>
			Instance(const Reader* reader);

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
			const ResourceBinding<Graphics::Buffer>* ConstantBuffer(size_t index)const;

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
			const ResourceBinding<Graphics::ArrayBuffer>* StructuredBuffer(size_t index)const;

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
			const ResourceBinding<Graphics::TextureSampler>* TextureSampler(size_t index)const;

		private:
			// Shader class used by the Instance
			Reference<const Graphics::ShaderClass> m_shader;

			// Constant buffer bindings
			std::vector<std::pair<const std::string*, Reference<ResourceBinding<Graphics::Buffer>>>> m_constantBuffers;

			// Structured buffer bindings
			std::vector<std::pair<const std::string*, Reference<ResourceBinding<Graphics::ArrayBuffer>>>> m_structuredBuffers;

			// Texture sampler bindings
			std::vector<std::pair<const std::string*, Reference<ResourceBinding<Graphics::TextureSampler>>>> m_textureSamplers;

			// CachedInstance has to access it's own internals
			friend class CachedInstance;
		};

		/// <summary>
		/// Cached instance of a material (Update() call is requred to update binding values)
		/// </summary>
		class JIMARA_API CachedInstance : public virtual Instance {
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

		class JIMARA_API Serializer : public virtual Serialization::SerializerList::From<Material> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Name of the GUID field </param>
			/// <param name="hint"> Feild description </param>
			/// <param name="attributes"> Item serializer attribute list </param>
			Serializer(
				const std::string_view& name = "Material", const std::string_view& hint = "Material bindings",
				const std::vector<Reference<const Object>>& attributes = {});

			/// <summary> Singleton instance of a Material serializer </summary>
			static const Serializer* Instance();

			/// <summary>
			/// Gives access to material fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Target material </param>
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Material* target)const override;
		};


	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// Shared lock for readers/writers
		mutable std::shared_mutex m_readWriteLock;

		// Shader class used by the Material
		Reference<const Graphics::ShaderClass> m_shaderClass;

		// ResourceBinding with a name
		template<typename ResourceType>
		class NamedResourceBinding : public virtual Graphics::ResourceBinding<ResourceType> {
		private:
			const std::string m_name;
		public:
			inline NamedResourceBinding(const std::string_view& name, ResourceType* boundObject = nullptr)
				: Graphics::ResourceBinding<ResourceType>(boundObject), m_name(name) {}
			inline virtual ~NamedResourceBinding() {}
			inline const std::string& Name()const { return m_name; }
		};

		// Constant buffer bindings
		std::unordered_map<std::string_view, Reference<NamedResourceBinding<Graphics::Buffer>>> m_constantBuffers;

		// Structured buffer bindings
		std::unordered_map<std::string_view, Reference<NamedResourceBinding<Graphics::ArrayBuffer>>> m_structuredBuffers;

		// Texture sampler bindings
		std::unordered_map<std::string_view, Reference<NamedResourceBinding<Graphics::TextureSampler>>> m_textureSamplers;

		// Shared instance
		mutable Reference<const Instance> m_sharedInstance;

		// Lock for preventing multi-initialisation of m_sharedInstance
		std::mutex mutable m_instanceLock;

		// Invoked, whenever any of the material properties gets altered
		mutable EventInstance<const Material*> m_onMaterialDirty;

		// Invoked, whenever the shared instance gets invalidated
		mutable EventInstance<const Material*> m_onInvalidateSharedInstance;

		// Some private stuff resides in here...
		struct Helpers;
	};

	// Type details
	template<> inline void TypeIdDetails::GetParentTypesOf<Material>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }
}
