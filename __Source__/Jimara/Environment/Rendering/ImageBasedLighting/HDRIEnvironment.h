#pragma once
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"


namespace Jimara {
	// __TODO__: Implement this crap!

	class JIMARA_API HDRIEnvironment : public virtual Resource {
	public:
		static Reference<HDRIEnvironment> Create(
			Graphics::GraphicsDevice* device, 
			Graphics::ShaderLoader* shaderLoader,
			Graphics::TextureSampler* hdri);

		inline Graphics::TextureSampler* HDRI()const { return m_hdriMap; }

		Graphics::TextureSampler* IrradianceMap()const { return m_irradianceMap; }

	private:
		const Reference<Graphics::TextureSampler> m_hdriMap;
		const Reference<Graphics::TextureSampler> m_irradianceMap;

		inline HDRIEnvironment(Graphics::TextureSampler* hdri, Graphics::TextureSampler* irradiance)
			: m_hdriMap(hdri), m_irradianceMap(irradiance) {}

		struct Helpers;
	};

	template<> inline void TypeIdDetails::GetParentTypesOf<HDRIEnvironment>(const Callback<TypeId>& report) {
		report(TypeId::Of<Resource>());
	};
}
