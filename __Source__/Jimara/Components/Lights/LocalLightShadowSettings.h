#pragma once
#include "../../Data/ConfigurableResource.h"
#include "../../Core/Systems/InputProvider.h"



namespace Jimara {
	/// <summary> Let the system know about the LocalLightShadowSettings class </summary>
	JIMARA_REGISTER_TYPE(Jimara::LocalLightShadowSettings);

	/// <summary> Settings for local light (like spot & point) shadowmaps </summary>
	class JIMARA_API LocalLightShadowSettings;

	/// <summary> Input provider for LocalLightShadowSettings </summary>
	using LocalLightShadowSettingsProvider = InputProvider<Reference<const LocalLightShadowSettings>>;

#pragma warning(disable: 4250)
	/// <summary> Settings for local light (like spot & point) shadowmaps </summary>
	class JIMARA_API LocalLightShadowSettings
		: public virtual ConfigurableResource
		, public virtual LocalLightShadowSettingsProvider
		, public virtual WeaklyReferenceable::StrongReferenceProvider {
	public:
		/// <summary> Constructor </summary>
		inline LocalLightShadowSettings() {}

		/// <summary> Constructor </summary>
		inline LocalLightShadowSettings(const ConfigurableResource::CreateArgs&) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~LocalLightShadowSettings() {}

		/// <summary> Returns self </summary>
		virtual std::optional<Reference<const LocalLightShadowSettings>> GetInput() final override { return this; };

		/// <summary> Resolution of the shadow (0 means no shadows) </summary>
		inline uint32_t ShadowResolution()const { return m_shadowResolution; }

		/// <summary>
		/// Sets the resolution of the shadow
		/// </summary>
		/// <param name="resolution"> Shadow resolution (0 means no shadows) </param>
		inline void SetShadowResolution(uint32_t resolution) { m_shadowResolution = resolution; }

		/// <summary> Tells, how soft the cast shadow is </summary>
		inline float ShadowSoftness()const { return m_shadowSoftness; }

		/// <summary>
		/// Sets shadow softness
		/// </summary>
		/// <param name="softness"> Softness (valid range is 0.0f to 1.0f) </param>
		inline void SetShadowSoftness(float softness) { m_shadowSoftness = Math::Min(Math::Max(0.0f, softness), 1.0f); }

		/// <summary> Tells, what size kernel is used for rendering soft shadows </summary>
		inline uint32_t ShadowFilterSize()const { return m_shadowSampleCount; }

		/// <summary>
		/// Sets blur filter size
		/// </summary>
		/// <param name="filterSize"> Gaussian filter size (odd numbers from 1 to 65 are allowed) </param>
		inline void SetShadowFilterSize(uint32_t filterSize) { m_shadowSampleCount = Math::Min(filterSize, 65u) | 1u; }

		/// <summary> Shadow distance from viewport origin, before it starts fading </summary>
		inline float ShadowDistance()const { return m_shadowDistance; }

		/// <summary>
		/// Sets shadow distance
		/// </summary>
		/// <param name="distance"> Shadow distance from viewport origin, before it starts fading </param>
		inline void SetShadowDistance(float distance) { m_shadowDistance = Math::Max(distance, 0.0f); }

		/// <summary> Shadow fade-out distance after ShadowDistance, before it fully disapears </summary>
		inline float ShadowFadeDistance()const { return m_shadowFadeDistance; }

		/// <summary>
		/// Sets shadow fade distance
		/// </summary>
		/// <param name="distance"> Shadow fade-out distance after ShadowDistance, before it fully disapears </param>
		inline void SetShadowFadeDistance(float distance) { m_shadowFadeDistance = Math::Max(distance, 0.0f); }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	private:
		uint32_t m_shadowResolution = 0u;
		float m_shadowDistance = 20.0f;
		float m_shadowFadeDistance = 10.0f;
		float m_shadowSoftness = 0.5f;
		uint32_t m_shadowSampleCount = 5u;

	public:
		/// <summary> Assigns holder to it's strong reference </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void FillWeakReferenceHolder(WeakReferenceHolder& holder)final override { holder = this; }

		/// <summary> Clears holder </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void ClearWeakReferenceHolder(WeakReferenceHolder& holder)final override { holder = nullptr; }

		/// <summary> Returns self </summary>
		virtual Reference<WeaklyReferenceable> RestoreStrongReference()final override { return this; }
	};
#pragma warning(default: 4250)

	// Type details
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<LocalLightShadowSettings>(const Callback<const Object*>& report);
	template<> inline void TypeIdDetails::GetParentTypesOf<LocalLightShadowSettings>(const Callback<TypeId>& report) {
		report(TypeId::Of<ConfigurableResource>());
		report(TypeId::Of<LocalLightShadowSettingsProvider>());
		report(TypeId::Of<WeaklyReferenceable::StrongReferenceProvider>());
	}
}
