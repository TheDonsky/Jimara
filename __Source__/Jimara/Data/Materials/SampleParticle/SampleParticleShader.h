#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleParticleShader);

	/// <summary>
	/// Test shader for particles
	/// </summary>
	class JIMARA_API SampleParticleShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance of the shader </summary>
		static SampleParticleShader* Instance();

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
		SampleParticleShader();

		// Some private stuff resides here...
		struct Helpers;
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleParticleShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleParticleShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleParticleShader::Instance()); }
}

