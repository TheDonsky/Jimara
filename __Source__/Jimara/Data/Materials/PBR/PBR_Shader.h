#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::PBR_Shader);

	class JIMARA_API PBR_Shader : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static PBR_Shader* Instance();

		/// <summary>
		/// Gets default constant buffer binding per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Constant buffer binding instance for the device </returns>
		virtual Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

		/// <summary>
		/// Gets default texture per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Texture binding instance for the device </returns>
		virtual Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

		/// <summary>
		/// Serializes shader bindings (like textures and constants)
		/// </summary>
		/// <param name="reportField"> 
		/// Each binding can be represented as an arbitrary SerializedObject (possibly with some nested entries); 
		/// Serialization scheme can use these to control binding set.
		/// </param>
		/// <param name="bindings"> Binding set to serialize </param>
		virtual void SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const override;

	private:
		// Constructor is private...
		PBR_Shader();

		// Some private stuff is here...
		struct Helpers;
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<PBR_Shader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<PBR_Shader>(const Callback<const Object*>& reportAttribute) { reportAttribute(PBR_Shader::Instance()); }
}
