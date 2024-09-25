#pragma once
#include "../../Material.h"
#include "../../../Environment/Scene/Scene.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleUIShader);

	/// <summary>
	/// Sample unlit shader for UI elements
	/// </summary>
	class JIMARA_API SampleUIShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static SampleUIShader* Instance();

		/// <summary>
		/// Default shared instance of a material based on this shader
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="bindlessBuffers"> Bindless buffer set </param>
		/// <param name="bindlessSamplers"> Bindless sampler set </param>
		/// <param name="shaders"> Shader set </param>
		/// <returns> 'Default' instance of this material for the configuration </returns>
		static Reference<const Material::Instance> MaterialInstance(
			Graphics::GraphicsDevice* device,
			Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
			Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
			const Material::LitShaderSet* shaders);

		/// <summary>
		/// Default shared instance of a material based on this shader
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> 'Default' instance of this material for the configuration </returns>
		inline static Reference<const Material::Instance> MaterialInstance(const SceneContext* context) {
			return (context == nullptr) ? nullptr : MaterialInstance(
				context->Graphics()->Device(),
				context->Graphics()->Bindless().Buffers(),
				context->Graphics()->Bindless().Samplers(),
				context->Graphics()->Configuration().ShaderLibrary()->LitShaders());
		}

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
		SampleUIShader();
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleUIShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleUIShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleUIShader::Instance()); }
}
