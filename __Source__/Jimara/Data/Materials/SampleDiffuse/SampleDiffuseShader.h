#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleDiffuseShader);

	/// <summary>
	/// Sample shader (applies simple diffuse shading)
	/// </summary>
	class SampleDiffuseShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static SampleDiffuseShader* Instance();

		/// <summary>
		/// Default singleton instance of a material base on this shader
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <returns> 'Default' instance of this material for the device </returns>
		static Reference<Material::Instance> MaterialInstance(Graphics::GraphicsDevice* device);

		/// <summary>
		/// Creates a material, bound to this shader
		/// </summary>
		/// <param name="texture"> Diffuse texture </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> New material </returns>
		static Reference<Material> CreateMaterial(Graphics::Texture* texture, Graphics::GraphicsDevice* device);

		/// <summary>
		/// Gets default texture per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Texture binding instance for the device </returns>
		virtual Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const override;

	private:
		// Constructor is private...
		SampleDiffuseShader();
	};

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<SampleDiffuseShader>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Graphics::ShaderClass>()); }
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleDiffuseShader>(const Callback<const Object*>& reportAttribute) { reportAttribute(SampleDiffuseShader::Instance()); }
}
