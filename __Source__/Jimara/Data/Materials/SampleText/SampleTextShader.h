#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleTextShader);

	/// <summary>
	/// Sample unlit shader for Text elements
	/// </summary>
	class JIMARA_API SampleTextShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static SampleTextShader* Instance();

		/// <summary>
		/// Default singleton instance of a material base on this shader for given device and color pair
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <returns> 'Default' instance of this material for the device </returns>
		static Reference<const Material::Instance> MaterialInstance(Graphics::GraphicsDevice* device);

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
		SampleTextShader();
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleTextShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleTextShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleTextShader::Instance()); }
}
