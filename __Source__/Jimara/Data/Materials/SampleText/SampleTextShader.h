#pragma once
#include "../../Materials/Material.h"
#include "../../../Environment/Scene/Scene.h"


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
		/// Default shared instance of a material based on this shader
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="bindlessBuffers"> Bindless buffer set </param>
		/// <param name="bindlessSamplers"> Bindless sampler set </param>
		/// <param name="shaders"> Shader collection </param>
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
		SampleTextShader();
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleTextShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleTextShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleTextShader::Instance()); }
}
