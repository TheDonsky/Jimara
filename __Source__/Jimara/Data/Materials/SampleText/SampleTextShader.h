#pragma once
#include "../../Materials/Material.h"
#include "../../../Environment/Scene/Scene.h"


namespace Jimara {
	/// <summary> Let the registry know about the shader </summary>
	JIMARA_REGISTER_TYPE(Jimara::SampleTextShader);

	/// <summary>
	/// Sample unlit shader for Text elements
	/// </summary>
	class JIMARA_API SampleTextShader final {
	public:
		/// <summary> Path to the sample text shader </summary>
		static const OS::Path PATH;

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

		/// <summary> Parameter name for the atlas texture (sampler2D) </summary>
		static const constexpr std::string_view ATLAS_TEXTURE_NAME = "atlasTexture";

	private:
		// Constructor is inaccessible...
		inline SampleTextShader() {}
	};
}
