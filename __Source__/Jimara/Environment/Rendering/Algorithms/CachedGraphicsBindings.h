#pragma once
#include "../../../Graphics/GraphicsDevice.h"


namespace Jimara {
	/// <summary>
	/// Some algorithms may require to create binding sets on the fly.
	/// This is a tool that will save bindings for corresponding shaders and give access when required
	/// </summary>
	class JIMARA_API CachedGraphicsBindings : public virtual Object {
	public:
		/// <summary>
		/// Creates cached binding collection
		/// </summary>
		/// <param name="shaders"> Shader bytecodes </param>
		/// <param name="shaderCount"> Number of shaders </param>
		/// <param name="search"> Binding Search functions </param>
		/// <param name="log"> Logger for error reporting (optional) </param>
		/// <returns> New instance of a CachedGraphicsBindings collection if successful </returns>
		static Reference<CachedGraphicsBindings> Create(
			const Graphics::SPIRV_Binary* const* shaders, size_t shaderCount, 
			const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log);

		/// <summary>
		/// Creates cached binding collection
		/// </summary>
		/// <param name="shader"> Shader bytecode </param>
		/// <param name="search"> Binding Search functions </param>
		/// <param name="log"> Logger for error reporting (optional) </param>
		/// <returns> New instance of a CachedGraphicsBindings collection if successful </returns>
		inline static Reference<CachedGraphicsBindings> Create(
			const Graphics::SPIRV_Binary* shader, 
			const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log) {
			return Create(&shader, 1u, search, log);
		}

		/// <summary> Virtual destructor </summary>
		inline virtual ~CachedGraphicsBindings() {}

		/// <summary> Number of used-up binding sets </summary>
		inline size_t BindingSetCount()const { return m_sets.Size(); }

		/// <summary>
		/// Generates search-functions per binding set id
		/// <para/> Result is valid, as long as the CachedGraphicsBindings instance is alive
		/// </summary>
		/// <param name="setId"> Binding set index (valid within [0 - BindingSetCount()) range) </param>
		/// <returns> Search functions </returns>
		inline Graphics::BindingSet::Descriptor::BindingSearchFunctions SearchFunctions(size_t setId)const { 
			return setId < BindingSetCount() ? m_sets[setId].SearchFunctions() : Graphics::BindingSet::Descriptor::BindingSearchFunctions{};
		}

	private:
		// Typedef for Reference<const Graphics::ResourceBinding<ResourceType>>
		template<typename ResourceType>
		using BindingReference = Reference<const Graphics::ResourceBinding<ResourceType>>;

		// Typedef for binding slot to BindingReference mapping
		template<typename ResourceType>
		using ResourceMappings = std::unordered_map<size_t, BindingReference<ResourceType>>;

		// Mappings per set
		struct SetBindings {
			ResourceMappings<Graphics::Buffer> constantBuffers;
			ResourceMappings<Graphics::ArrayBuffer> structuredBuffers;
			ResourceMappings<Graphics::TextureSampler> textureSamplers;
			ResourceMappings<Graphics::TextureView> textureViews;
			ResourceMappings<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> bindlessBuffers;
			ResourceMappings<Graphics::BindlessSet<Graphics::TextureSampler>::Instance> bindlessSamplers;

			bool Build(
				const Graphics::SPIRV_Binary* const* shaders, size_t shaderCount, size_t setId,
				const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log);
			template<typename Type>
			struct Find { typedef BindingReference<Type>(*Fn)(const SetBindings*, const Graphics::BindingSet::BindingDescriptor); };
			Graphics::BindingSet::Descriptor::BindingSearchFunctions SearchFunctions()const;
		};

		// Mappings per set
		Stacktor<SetBindings, 4u> m_sets;

		// Constructor is private
		inline CachedGraphicsBindings() {}
	};
}
