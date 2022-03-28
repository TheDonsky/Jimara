#pragma once
#include "ShaderResourceBindings.h"
#include "../../../Data/Serialization/ItemSerializers.h"
#include "../../../Core/Helpers.h"
#include "../../../OS/IO/Path.h"
#include <set>



namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader class description that helps with shader binary loading, default bindings ans similar stuff
		/// <para /> Note: 
		///		<para />To register a new/custom ShaderClass object, 
		///		<para />just register type with:
		///			<code>JIMARA_REGISTER_TYPE(Namespace::ShaderClassType); </code>
		///		<para /> and report singleton instance with TypeIdDetails: 
		///			<code> <para /> namespace Jimara {
		///				<para /> template&lt;&gt; inline void TypeIdDetails::GetTypeAttributesOf&lt;Namespace::ShaderClassType&gt;(const Callback&lt;const Object*&gt;&#38; report) {
		///					<para /> static const Namespace::ShaderClassType instance; // You can create singleton in any way you wish, btw...
		///					<para /> report(&#38;instance);
		///				<para /> }
		///			<para /> } </code>
		///		<para /> Doing these two sets will cause your class instance to appear in ShaderClass::Set::All(), meaning, it'll be accessible to editor and serializers
		/// </summary>
		class ShaderClass : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="shaderPath"> Shader path within the project directory </param>
			ShaderClass(const OS::Path& shaderPath);

			/// <summary> Shader path within the project directory </summary>
			const OS::Path& ShaderPath()const;

			/// <summary> Short for ShaderResourceBindings::ConstantBufferBinding </summary>
			typedef ShaderResourceBindings::ConstantBufferBinding ConstantBufferBinding;

			/// <summary> Short for ShaderResourceBindings::StructuredBufferBinding </summary>
			typedef ShaderResourceBindings::StructuredBufferBinding StructuredBufferBinding;

			/// <summary> Short for ShaderResourceBindings::TextureSamplerBinding </summary>
			typedef ShaderResourceBindings::TextureSamplerBinding TextureSamplerBinding;

			/// <summary> Descriptor of a shader class binding set (could be something like a material writer) </summary>
			struct Bindings;

			/// <summary> Set of ShaderClass objects </summary>
			class Set;

			/// <summary>
			/// Gets default constant buffer binding per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Constant buffer binding instance for the device </returns>
			virtual inline Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string_view& name, GraphicsDevice* device)const { Unused(name, device); return nullptr; }

			/// <summary>
			/// Gets default structured buffer binding per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Structured buffer binding instance for the device </returns>
			virtual inline Reference<const StructuredBufferBinding> DefaultStructuredBufferBinding(const std::string_view& name, GraphicsDevice* device)const { Unused(name, device); return nullptr; }
			
			/// <summary>
			/// Gets default texture per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Texture binding instance for the device </returns>
			virtual inline Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string_view& name, GraphicsDevice* device)const { Unused(name, device); return nullptr; }

			/// <summary>
			/// Serializes shader bindings (like textures and constants)
			/// </summary>
			/// <param name="reportField"> 
			/// Each binding can be represented as an arbitrary SerializedObject (possibly with some nested entries); 
			/// Serialization scheme can use these to control binding set.
			/// </param>
			/// <param name="bindings"> Binding set to serialize </param>
			virtual inline void SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const { Unused(reportField, bindings); }

		private:
			// Shader path within the project directory
			const OS::Path m_shaderPath;

			// Shader path as string;
			const std::string m_pathStr;
		};

		/// <summary> Descriptor of a shader class binding set (could be something like a material writer) </summary>
		struct ShaderClass::Bindings {
			/// <summary> Graphics device, binding set is tied to </summary>
			virtual inline Graphics::GraphicsDevice* GraphicsDevice()const = 0;

			/// <summary>
			/// Constant buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <returns> Constant buffer binding if found, nullptr otherwise </returns>
			virtual Graphics::Buffer* GetConstantBuffer(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates constant buffer binding
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetConstantBuffer(const std::string_view& name, Graphics::Buffer* buffer) = 0;

			/// <summary>
			/// Structured buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <returns> Structured buffer binding if found, nullptr otherwise </returns>
			virtual Graphics::ArrayBuffer* GetStructuredBuffer(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates structured buffer binding
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetStructuredBuffer(const std::string_view& name, Graphics::ArrayBuffer* buffer) = 0;

			/// <summary>
			/// Texture sampler binding by name
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <returns> Texture sampler binding if found, nullptr otherwise </returns>
			virtual Graphics::TextureSampler* GetTextureSampler(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates texture sampler binding
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <param name="buffer"> New sampler to set (nullptr removes the binding) </param>
			virtual void SetTextureSampler(const std::string_view& name, Graphics::TextureSampler* sampler) = 0;
		};

		/// <summary>
		/// Set of ShaderClass objects
		/// </summary>
		class ShaderClass::Set : public virtual Object {
		public:
			/// <summary>
			///  Set of all currently registered ShaderClass objects
			/// Notes: 
			///		0. Value will automagically change whenever any new type gets registered or unregistered;
			///		1. ShaderClass::Set is immutable, so no worries about anything going out of scope or deadlocking.
			/// </summary>
			/// <returns> Reference to current set of all </returns>
			static Reference<Set> All();

			/// <summary> Number of shaders within the set </summary>
			size_t Size()const;

			/// <summary>
			/// ShaderClass by index
			/// </summary>
			/// <param name="index"> ShaderClass index </param>
			/// <returns> ShaderClass </returns>
			const ShaderClass* At(size_t index)const;

			/// <summary>
			/// ShaderClass by index
			/// </summary>
			/// <param name="index"> ShaderClass index </param>
			/// <returns> ShaderClass </returns>
			const ShaderClass* operator[](size_t index)const;

			/// <summary>
			/// Finds ShaderClass by it's ShaderPath() value
			/// </summary>
			/// <param name="shaderPath"> Shader path within the code </param>
			/// <returns> ShaderClass s, such that s->ShaderPath() is shaderPath if found, nullptr otherwise </returns>
			const ShaderClass* FindByPath(const OS::Path& shaderPath)const;

			/// <summary> 'Invalid' shader class index </summary>
			inline static size_t NoIndex() { return ~((size_t)0); }

			/// <summary>
			/// Finds index of a shader class
			/// </summary>
			/// <param name="shaderClass"> Shader class </param>
			/// <returns> Index i, such that this->At(i) is shaderClass; NoIndex if not found </returns>
			size_t IndexOf(const ShaderClass* shaderClass)const;

			/// <summary> Shader class selector </summary>
			typedef Serialization::ItemSerializer::Of<const ShaderClass*> ShaderClassSerializer;

			/// <summary> Shader class selector (Alive and valid only while this set exists..) </summary>
			const ShaderClassSerializer* ShaderClassSelector()const { return m_classSelector; }

		private:
			// List of all known shaders
			const std::vector<Reference<const ShaderClass>> m_shaders;

			// Index of each shader
			typedef std::unordered_map<const ShaderClass*, size_t> IndexPerShader;
			const IndexPerShader m_indexPerShader;

			// Mapping from ShaderClass::ShaderPath() to corresponding ShaderClass()
			typedef std::unordered_map<OS::Path, const ShaderClass*> ShadersByPath;
			const ShadersByPath m_shadersByPath;

			// Shader class selector
			const Reference<const ShaderClassSerializer> m_classSelector;

			// Constructor
			Set(const std::set<Reference<const ShaderClass>>& shaders);
		};
	}

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass::Set>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
}
