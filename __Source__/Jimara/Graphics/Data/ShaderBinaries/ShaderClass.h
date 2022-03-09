#pragma once
#include "ShaderResourceBindings.h"
#include "../../../OS/IO/Path.h"



namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader class description that helps with shader binary loading, default bindings ans similar stuff
		/// Note: 
		///		To register a new/custom ShaderClass object, 
		///		just register type with:
		///			"JIMARA_REGISTER_TYPE(Namespace::ShaderClassType);"
		///		and report singleton instance with TypeIdDetails: 
		///			"
		///			namespace Jimara {
		///				template<> inline void TypeIdDetails::GetTypeAttributesOf<Namespace::ShaderClassType>(const Callback<const Object*>& report) {
		///					static const Namespace::ShaderClassType instance; // You can create singleton in any way you wish, btw...
		///					report(&instance);
		///				}
		///			}
		///			".
		///		Doing these two sets will cause your class instance to appear in ShaderClass::Set::All(), meaning, it'll be accessible to editor and serializers
		/// </summary>
		class ShaderClass : public virtual Object {
		public:
			ShaderClass(const OS::Path& shaderPath);

			const OS::Path& ShaderPath()const;

			typedef ShaderResourceBindings::ConstantBufferBinding ConstantBufferBinding;

			typedef ShaderResourceBindings::StructuredBufferBinding StructuredBufferBinding;

			typedef ShaderResourceBindings::TextureSamplerBinding TextureSamplerBinding;

			virtual inline Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }

			virtual inline Reference<const StructuredBufferBinding> DefaultStructuredBufferBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }
			
			virtual inline Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string& name, GraphicsDevice* device)const { return nullptr; }

			/// <summary> Set of ShaderClass objects </summary>
			class Set;

		private:
			const OS::Path m_shaderPath;
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

		private:
			// List of all known shaders
			const std::vector<Reference<const ShaderClass>> m_shaders;

			// Mapping from ShaderClass::ShaderPath() to corresponding ShaderClass()
			typedef std::unordered_map<OS::Path, const ShaderClass*> ShadersByPath;
			const ShadersByPath m_shadersByPath;

			// Constructor
			Set(const std::unordered_set<Reference<const ShaderClass>>& shaders);
		};
	}

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass::Set>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
}
