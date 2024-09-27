#pragma once
#include "../../Materials/Material.h"
#include "../../../Environment/Scene/Scene.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleDiffuseShader);

	/// <summary>
	/// Sample shader tools (applies simple diffuse shading)
	/// </summary>
	class JIMARA_API SampleDiffuseShader final {
	public:
		/// <summary> Path to the sample diffuse shader </summary>
		static const OS::Path PATH;

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

		/// <summary> Base-Color parameter name (vec3) </summary>
		static const constexpr std::string_view COLOR_NAME = "baseColor";

		/// <summary> Diffuse-texture parameter name (sampler2D) </summary>
		static const constexpr std::string_view DIFFUSE_NAME = "colorTexture";

		/// <summary> Normal map parameter name (sampler2D) </summary>
		static const constexpr std::string_view NORMAL_MAP_NAME = "normalMap";

	private:
		// Constructor is inaccessible...
		inline SampleDiffuseShader() {}
	};
}
