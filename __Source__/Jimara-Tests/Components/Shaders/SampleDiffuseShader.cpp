#include "SampleDiffuseShader.h"


namespace Jimara {
	namespace Test {
		SampleDiffuseShader* SampleDiffuseShader::Instance() {
			static SampleDiffuseShader instance;
			return &instance;
		}

		Reference<Material> SampleDiffuseShader::CreateMaterial(Graphics::Texture* texture) {
			Reference<Material> material = Object::Instantiate<Material>();
			Material::Writer writer(material);
			writer.SetShader(Instance());
			writer.SetTextureSampler("texSampler", texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler());
			return material;
		}

		SampleDiffuseShader::SampleDiffuseShader() : Graphics::ShaderClass("Jimara-Tests/Components/Shaders/Test_SampleDiffuseShader") {}
	}
}
