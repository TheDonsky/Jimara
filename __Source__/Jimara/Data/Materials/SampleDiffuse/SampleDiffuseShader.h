#pragma once
#include "../../Materials/Material.h"
#include "../../../Environment/Scene/Scene.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleDiffuseShader);

	/// <summary>
	/// Sample shader (applies simple diffuse shading)
	/// </summary>
	class JIMARA_API SampleDiffuseShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static SampleDiffuseShader* Instance();

		/// <summary> Path to the sample diffuse shader </summary>
		static const constexpr std::string_view PATH = "Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader";

		/// <summary>
		/// Retrieves shared material instance of given color
		/// </summary>
		/// <param name="device"> Graphic device </param>
		/// <param name="bindlessBuffers"> Bindless buffer set </param>
		/// <param name="bindlessSamplers"> Bindless sampler set </param>
		/// <param name="shaders"> Shader set </param>
		/// <param name="baseColor"> Material color </param>
		/// <returns> 'Default' instance of this material for the color </returns>
		static Reference<const Material::Instance> MaterialInstance(
			Graphics::GraphicsDevice* device,
			Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
			Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
			const Material::LitShaderSet* shaders,
			Vector3 baseColor = Vector3(1.0f));

		/// <summary>
		/// Default singleton instance of a material base on this shader for given device and color pair
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="baseColor"> Material color </param>
		/// <returns> 'Default' instance of this material for the device </returns>
		static Reference<const Material::Instance> MaterialInstance(const SceneContext* context, Vector3 baseColor = Vector3(1.0f));

		/// <summary>
		/// Creates a material, bound to this shader
		/// </summary>
		/// <param name="device"> Graphic device </param>
		/// <param name="bindlessBuffers"> Bindless buffer set </param>
		/// <param name="bindlessSamplers"> Bindless sampler set </param>
		/// <param name="shaders"> Shaderl set </param>
		/// <param name="texture"> Diffuse texture </param>
		/// <returns> New material </returns>
		static Reference<Material> CreateMaterial(
			Graphics::GraphicsDevice* device,
			Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
			Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
			const Material::LitShaderSet* shaders,
			Graphics::Texture* texture);

		/// <summary>
		/// Creates a material, bound to this shader
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="texture"> Diffuse texture </param>
		/// <returns> New material </returns>
		static Reference<Material> CreateMaterial(const SceneContext* context, Graphics::Texture* texture);

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
		SampleDiffuseShader();
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleDiffuseShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleDiffuseShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleDiffuseShader::Instance()); }
}
