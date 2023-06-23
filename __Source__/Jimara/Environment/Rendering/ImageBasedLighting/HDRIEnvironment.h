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

		inline Graphics::TextureSampler* IrradianceMap()const { return m_irradianceMap; }

		inline Graphics::TextureSampler* PreFilteredMap()const { return m_preFilteredMap; }

	private:
		const Reference<Graphics::TextureSampler> m_hdriMap;
		const Reference<Graphics::TextureSampler> m_irradianceMap;
		const Reference<Graphics::TextureSampler> m_preFilteredMap;

		inline HDRIEnvironment(
			Graphics::TextureSampler* hdri, 
			Graphics::TextureSampler* irradiance, 
			Graphics::TextureSampler* preFiltered)
			: m_hdriMap(hdri)
			, m_irradianceMap(irradiance)
			, m_preFilteredMap(preFiltered) {}

		struct Helpers;
	};

	template<> inline void TypeIdDetails::GetParentTypesOf<HDRIEnvironment>(const Callback<TypeId>& report) {
		report(TypeId::Of<Resource>());
	};
}
