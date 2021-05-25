#pragma once
#include "Data/Material.h"


namespace Jimara {
	namespace Test {
		class SampleDiffuseShader : public virtual Graphics::ShaderClass {
		public:
			static SampleDiffuseShader* Instance();

			static Reference<Material> CreateMaterial(Graphics::Texture* texture);

		private:
			SampleDiffuseShader();
		};
	}
}
