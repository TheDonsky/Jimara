#pragma once
#include "../../Materials/Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::PBR_ColorShader);

	class JIMARA_API PBR_ColorShader : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static PBR_ColorShader* Instance();

		/// <summary>
		/// Gets default constant buffer binding per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Constant buffer binding instance for the device </returns>
		virtual Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

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
		PBR_ColorShader();

		// Some private stuff is here...
		struct Helpers;
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<PBR_ColorShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<PBR_ColorShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(PBR_ColorShader::Instance()); }
}
