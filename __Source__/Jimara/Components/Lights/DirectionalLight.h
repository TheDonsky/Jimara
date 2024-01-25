#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightDescriptor.h"
#include "../../Data/ConfigurableResource.h"
#include "../../Core/Systems/InputProvider.h"
#include "../Camera.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::DirectionalLight);
	JIMARA_REGISTER_TYPE(Jimara::DirectionalLight::ShadowSettings);
	
	/// <summary>
	/// Directional light component
	/// </summary>
	class JIMARA_API DirectionalLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the light component </param>
		/// <param name="color"> Light color </param>
		DirectionalLight(Component* parent, const std::string_view& name = "DirectionalLight", Vector3 color = Vector3(1.0f, 1.0f, 1.0f));

		/// <summary> Virtual destructor </summary>
		virtual ~DirectionalLight();

		/// <summary> Directional light color </summary>
		Vector3 Color()const;

		/// <summary>
		/// Sets light color
		/// </summary>
		/// <param name="color"> New color </param>
		void SetColor(Vector3 color);

		/// <summary> Color multiplier </summary>
		inline float Intensity()const { return m_intensity; }

		/// <summary>
		/// Sets intensity
		/// </summary>
		/// <param name="intensity"> Color multiplier </param>
		inline void SetIntensity(float intensity) { m_intensity = Math::Max(intensity, 0.0f); }

		/// <summary> Optionally, the spotlight projection can take color form this texture </summary>
		inline Graphics::TextureSampler* Texture()const { return m_projectedTexture; }

		/// <summary>
		/// Sets projected texture
		/// </summary>
		/// <param name="texture"> Texture for coloring spot non-uniformally (can be nullptr) </param>
		inline void SetTexture(Graphics::TextureSampler* texture) { m_projectedTexture = texture; }

		/// <summary> Projection texture tiling (ignored if there is no texture) </summary>
		inline Vector2 TextureTiling()const { return m_projectedTextureTiling; }

		/// <summary>
		/// Sets projected texture tiling
		/// </summary>
		/// <param name="tiling"> Tells, how many times should the texture repeat itself </param>
		inline void SetTextureTiling(const Vector2& tiling) { m_projectedTextureTiling = tiling; }

		/// <summary> Projection texture offset (ignored if there is no texture) </summary>
		inline Vector2 TextureOffset()const { return m_projectedTextureOffset; }

		/// <summary>
		/// Sets projected texture offset
		/// </summary>
		/// <param name="tiling"> Tells, how to shift the texture around </param>
		inline void SetTextureOffset(const Vector2& offset) { m_projectedTextureOffset = offset; }

		/// <summary> Directional light shadow settings </summary>
		class JIMARA_API ShadowSettings;

		/// <summary> Shadow cascade information </summary>
		class JIMARA_API ShadowCascadeInfo;

		/// <summary> Provider input for shadow settings </summary>
		using ShadowSettingsProvider = InputProvider<Reference<const DirectionalLight::ShadowSettings>>;

		/// <summary> Shadow settings provider </summary>
		inline Reference<ShadowSettingsProvider> GetShadowSettings()const { return m_shadowSettings; }

		/// <summary>
		/// Sets shadow settings provider
		/// </summary>
		/// <param name="provider"> Shadow Settings input </param>
		inline void SetShadowSettings(ShadowSettingsProvider* provider) { m_shadowSettings = provider; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

	private:
		// Set of all lights from the scene
		const Reference<LightDescriptor::Set> m_allLights;

		// Light color
		Vector3 m_color;

		// Color multipler
		float m_intensity = 1.0f;

		// Projected texture
		Reference<Graphics::TextureSampler> m_projectedTexture;
		Vector2 m_projectedTextureTiling = Vector2(1.0f);
		Vector2 m_projectedTextureOffset = Vector2(0.0f);

		// Shadow settings
		WeakReference<ShadowSettingsProvider> m_shadowSettings;
		const Reference<ShadowSettings> m_defaultShadowSettings;

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

		// Some private helper stuff here
		struct Helpers;
	};


	/// <summary> Directional light shadow cascade information </summary>
	class JIMARA_API DirectionalLight::ShadowCascadeInfo {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="size"> Cascade size </param>
		/// <param name="blendSize"> Size that should be blended with the next cascade </param>
		inline ShadowCascadeInfo(float size = 10.0f, float blendSize = 0.01f) : m_size(size), m_blendSize(blendSize) {  }

		/// <summary> Cascade size in units </summary>
		inline float Size()const { return m_size; }

		/// <summary>
		/// Sets cascade size
		/// </summary>
		/// <param name="size"> Cascade size </param>
		inline void SetSize(float size) { m_size = Math::Max(size, 0.0f); SetBlendSize(BlendSize()); }

		/// <summary> Size that should be blended with the next cascade </summary>
		inline float BlendSize()const { return m_blendSize; }

		/// <summary>
		/// Sets blend size
		/// </summary>
		/// <param name="blendSize"> Size that should be blended with the next cascade </param>
		inline void SetBlendSize(float blendSize) { m_blendSize = Math::Min(Math::Max(blendSize, 0.0f), Size()); }

		/// <summary> Default serializer of ShadowCascadeInfo </summary>
		class JIMARA_API Serializer : public virtual Serialization::SerializerList::From<ShadowCascadeInfo> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Field name </param>
			/// <param name="hint"> Field hint </param>
			/// <param name="attributes"> Additional attributes </param>
			inline Serializer(const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer(name, hint, attributes) {}

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			/// <param name="target"> ShadowCascadeInfo </param>
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ShadowCascadeInfo* target)const override;
		};

	private:
		float m_size;
		float m_blendSize;
	};

#pragma warning(disable: 4250)
	/// <summary> Directional light shadow settings </summary>
	class JIMARA_API DirectionalLight::ShadowSettings 
		: public virtual ConfigurableResource
		, public virtual DirectionalLight::ShadowSettingsProvider
		, public virtual WeaklyReferenceable::StrongReferenceProvider {
	public:
		/// <summary> Constructor </summary>
		inline ShadowSettings() {}

		/// <summary> Constructor </summary>
		inline ShadowSettings(const ConfigurableResource::CreateArgs&) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ShadowSettings() {}

		/// <summary> Returns self </summary>
		virtual std::optional<Reference<const ShadowSettings>> GetInput() final override { return this; };
		
		/// <summary> Resolution of the shadow (0 means no shadows) </summary>
		inline uint32_t ShadowResolution()const { return m_shadowResolution; }

		/// <summary>
		/// Sets the resolution of the shadow
		/// </summary>
		/// <param name="resolution"> Shadow resolution (0 means no shadows) </param>
		inline void SetShadowResolution(uint32_t resolution) { m_shadowResolution = resolution; }

		/// <summary> Shadow renderer far plane </summary>
		inline float ShadowRange()const { return m_shadowRange; }

		/// <summary>
		/// Sets shadow range
		/// </summary>
		/// <param name="range"> Shadow renderer far plane </param>
		inline void SetShadowRange(float range) { m_shadowRange = Math::Max(0.0f, range); }

		/// <summary> Fraction of the light, still present in the shadowed areas </summary>
		inline float AmbientLightAmount()const { return m_ambientLightAmount; }

		/// <summary>
		/// Sets the amount of ambient light
		/// </summary>
		/// <param name="ambientLight"> AmbientLightAmount (valid range is 0.0f to 1.0f) </param>
		inline void SetAmbientLightAmount(float ambientLight) { m_ambientLightAmount = Math::Min(Math::Max(0.0f, ambientLight), 1.0f); }

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

		/// <summary> 
		/// VSM can have some visual artifacts with overlapping obscurers. 
		/// This value supresses the artifacts, but has negative impact on softness.
		/// </summary>
		inline float BleedingReduction()const { return m_bleedingReduction; }

		/// <summary>
		/// Sets bleeding reduction factor
		/// </summary>
		/// <param name="amount"> Bleeding Reduction amount </param>
		inline void SetBleedingReduction(float amount) { m_bleedingReduction = Math::Min(Math::Max(0.0f, amount), 1.0f); }

		/// <summary> Number of cascades </summary>
		inline size_t CascadeCount()const { return m_cascades.Size(); }

		/// <summary>
		/// Sets cascade count
		/// </summary>
		/// <param name="count"> Number of cascades to use (values are only valid within 1-4 range) </param>
		inline void SetCascadeCount(size_t count) { m_cascades.Resize(Math::Min(Math::Max(count, size_t(1u)), size_t(4u))); }

		/// <summary>
		/// Cascade by index
		/// </summary>
		/// <param name="index"> Cascade index </param>
		/// <returns> Cascade info </returns>
		const ShadowCascadeInfo& Cascade(size_t index)const { return m_cascades[index]; }

		/// <summary>
		/// Cascade by index
		/// </summary>
		/// <param name="index"> Cascade index </param>
		/// <returns> Cascade info </returns>
		ShadowCascadeInfo& Cascade(size_t index) { return m_cascades[index]; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	private:
		uint32_t m_shadowResolution = 0u;
		float m_shadowRange = 100.0f;
		float m_ambientLightAmount = 0.25f;
		float m_bleedingReduction = 0.125f;
		float m_shadowSoftness = 0.5f;
		uint32_t m_shadowSampleCount = 5u;
		Stacktor<ShadowCascadeInfo, 4u> m_cascades = Stacktor<ShadowCascadeInfo, 4u>({
			ShadowCascadeInfo(16.0f, 1.0f),
			ShadowCascadeInfo(32.0f, 2.0f),
			ShadowCascadeInfo(64.0f, 32.0f)
		});

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

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<DirectionalLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<DirectionalLight>(const Callback<const Object*>& report);
	template<> inline void TypeIdDetails::GetParentTypesOf<DirectionalLight::ShadowSettings>(const Callback<TypeId>& report) { 
		report(TypeId::Of<ConfigurableResource>()); 
		report(TypeId::Of<DirectionalLight::ShadowSettingsProvider>());
		report(TypeId::Of<WeaklyReferenceable::StrongReferenceProvider>());
	}
}
