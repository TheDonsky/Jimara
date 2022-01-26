#pragma once
#include "../../Material.h"


namespace Jimara {
	/// <summary>
	/// Sample shader (applies simple diffuse shading)
	/// </summary>
	class SampleDiffuseShader final : public virtual Graphics::ShaderClass {
	public:
		/// <summary> Singleton instance </summary>
		static SampleDiffuseShader* Instance();

		/// <summary> Default singleton instance of a material base on this shader </summary>
		static Material::Instance* MaterialInstance();

		/// <summary>
		/// Creates a material, bound to this shader
		/// </summary>
		/// <param name="texture"> Diffuse texture </param>
		/// <returns> New material </returns>
		static Reference<Material> CreateMaterial(Graphics::Texture* texture);

		/// <summary>
		/// Gets default testure per device
		/// </summary>
		/// <param name="name"> Binding name </param>
		/// <param name="device"> Graphics device </param>
		/// <returns> Texture binding instance for the device </returns>
		virtual Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string& name, Graphics::GraphicsDevice* device)const override;

	private:
		// Constructor is private...
		SampleDiffuseShader();
	};
}