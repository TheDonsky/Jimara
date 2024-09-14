#pragma once
#include "../../../Graphics/GraphicsDevice.h"
#include "../../../Graphics/Pipeline/OneTimeCommandPool.h"
#include "../../../Data/ShaderLibrary.h"


namespace Jimara {
	/// <summary> 
	/// Environment maps, generated from HDRI image
	/// <para/> Notes:
	/// <para/>		0. Implementation derived from https://learnopengl.com/PBR/IBL/Diffuse-irradiance and https://learnopengl.com/PBR/IBL/Specular-IBL
	/// <para/>		1. HDRIEnvironment is more or less tightly coupled with PBR shaders and it may not work well, if at all with different surface models.
	/// </summary>
	class JIMARA_API HDRIEnvironment : public virtual Resource {
	public:
		/// <summary>
		/// Creates HDRIEnvironment maps
		/// <para/> Keep in mind, that Create uses an internal command buffer and waits on it. 
		/// Because of that, it is higly recommended to create/load new instances from and asynchronous thread to avoid hitches during runtime
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader binary loader </param>
		/// <param name="hdri"> HDRI map to do precaluclations for </param>
		/// <returns> New instance of an HDRIEnvironment </returns>
		static Reference<HDRIEnvironment> Create(
			Graphics::GraphicsDevice* device, 
			ShaderLibrary* shaderLibrary,
			Graphics::TextureSampler* hdri);

		/// <summary>
		/// Gives access to shared BRDF intergration map
		/// <para/> Keep in mind, that initial creation uses an internal command buffer and waits on it. 
		/// Because of that, it is higly recommended to create/load new instances from and asynchronous thread to avoid hitches during runtime
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLibrary"> Shader binary loader </param>
		/// <returns> Shared instance of a BRDF integration map </returns>
		static Reference<Graphics::TextureSampler> BrdfIntegrationMap(
			Graphics::GraphicsDevice* device,
			ShaderLibrary* shaderLibrary);

		/// <summary> (source) HDRI texture </summary>
		inline Graphics::TextureSampler* HDRI()const { return m_hdriMap; }

		/// <summary> Diffuse irradiance map (https://learnopengl.com/PBR/IBL/Diffuse-irradiance) </summary>
		inline Graphics::TextureSampler* IrradianceMap()const { return m_irradianceMap; }

		/// <summary> Specular pre-filtered map (https://learnopengl.com/PBR/IBL/Specular-IBL) </summary>
		inline Graphics::TextureSampler* PreFilteredMap()const { return m_preFilteredMap; }

		/// <summary> 
		/// BRDF integration map 
		/// (Same as HDRIEnvironment::BrdfIntegrationMap; required by PBR shaders for IBL; https://learnopengl.com/PBR/IBL/Specular-IBL) 
		/// </summary>
		inline Graphics::TextureSampler* BrdfIntegrationMap()const { return m_brdfIntegrationMap; }

	private:
		// (source) HDRI texture
		const Reference<Graphics::TextureSampler> m_hdriMap;

		// Diffuse irradiance map
		const Reference<Graphics::TextureSampler> m_irradianceMap;

		// Specular pre-filtered map
		const Reference<Graphics::TextureSampler> m_preFilteredMap;

		// BRDF integration map
		const Reference<Graphics::TextureSampler> m_brdfIntegrationMap;

		// One Time Command buffer pool, used for generation
		const Reference<Graphics::OneTimeCommandPool> m_commandBufferPool;

		// Constructor is private
		inline HDRIEnvironment(
			Graphics::TextureSampler* hdri,
			Graphics::TextureSampler* irradiance,
			Graphics::TextureSampler* preFiltered,
			Graphics::TextureSampler* brdfIntegrationMap,
			Graphics::OneTimeCommandPool* oneTimePool)
			: m_hdriMap(hdri)
			, m_irradianceMap(irradiance)
			, m_preFilteredMap(preFiltered)
			, m_brdfIntegrationMap(brdfIntegrationMap)
			, m_commandBufferPool(oneTimePool) {}

		// Actual implementation resides in here
		struct Helpers;
	};

	/// <summary> Parent types </summary>
	template<> inline void TypeIdDetails::GetParentTypesOf<HDRIEnvironment>(const Callback<TypeId>& report) {
		report(TypeId::Of<Resource>());
	};
}
