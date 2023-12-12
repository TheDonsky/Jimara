#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/LightingModels/LightingModel.h"


namespace Jimara {
	/// <summary> This will make sure, BloomEffect is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::BloomEffect);

	/// <summary>
	/// Bloom post process effect
	/// </summary>
	class JIMARA_API BloomEffect : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the camera component </param>
		BloomEffect(Component* parent, const std::string_view& name = "Bloom");

		/// <summary> Virtual destructor </summary>
		virtual ~BloomEffect();

		/// <summary> Amount of bloom applied to final image </summary>
		inline float Strength()const { return m_strength; }

		/// <summary>
		/// Sets "strength" of bloom effect
		/// </summary>
		/// <param name="strength"> Kernel strength parameter </param>
		inline void SetStrength(float strength) { m_strength = Math::Max(0.0f, strength); }

		/// <summary> Size of bloom effect </summary>
		inline float Size()const { return m_size; }

		/// <summary>
		/// Sets size of the bloom effect
		/// </summary>
		/// <param name="spread"> Kernel size parameter </param>
		inline void SetSize(float size) { m_size = Math::Min(Math::Max(0.0f, size), 1.0f); }

		/// <summary> Minimal pixel intensity for it to start "blooming" (negative values mean 'no thresholding') </summary>
		inline float Threshold()const { return m_threshold; }

		/// <summary>
		/// Sets threshold for the bloom effect
		/// </summary>
		/// <param name="intensity"> Bloom threshold </param>
		inline void SetThreshold(float intensity) { m_threshold = intensity; }

		/// <summary> Bloom will gradually fade in and out between intensities equal to threshold and (threshold + thresholdSize) </summary>
		inline float ThresholdSize()const { return m_thresholdSize; }

		/// <summary>
		/// Sets threshold size for the bloom effect
		/// </summary>
		/// <param name="fade"> Bloom threshold size </param>
		inline void SetThresholdSize(float fade) { m_thresholdSize = Math::Max(fade, 0.0f); }

		/// <summary> Input color channel values will be clamped to this to avoid 'exploding-infinite intencity pixels' from ruining the image </summary>
		inline float MaxChannelIntensity()const { return m_maxChannelIntensity; }

		/// <summary>
		/// Sets max color value
		/// </summary>
		/// <param name="value"> Input color channel values will be clamped to this to avoid 'exploding-infinite intencity pixels' from ruining the image </param>
		inline void SetMaxChannelIntensity(float value) { m_maxChannelIntensity = Math::Max(value, 0.0f); }

		/// <summary> Dirt texture, that will show up as an overly on bloomed areas (optional) </summary>
		inline Graphics::TextureSampler* DirtTexture()const { return m_dirtTexture; }

		/// <summary>
		/// Sets dirt texture for bloom
		/// </summary>
		/// <param name="dirt"> Dirt texture </param>
		inline void SetDirtTexture(Graphics::TextureSampler* dirt) { m_dirtTexture = dirt; }

		/// <summary> Dirt texture intensity (ignored if there's no DirtTexture set) </summary>
		inline float DirtStrength()const { return m_dirtStrength; }

		/// <summary>
		/// Sets dirt texture strength/intensity
		/// </summary>
		/// <param name="intensity"> DirtStrength </param>
		inline void SetDirtStrength(float intensity) { m_dirtStrength = intensity; }

		/// <summary> Tiling for the dirt texture (if it's present) </summary>
		inline Vector2 DirtTextureTiling()const { return m_dirtTiling; }

		/// <summary>
		/// Applies tiling to dirt texture (if it's present)
		/// </summary>
		/// <param name="tiling"> Texture scale </param>
		inline void SetDirtTextureTiling(const Vector2& tiling) { m_dirtTiling = tiling; }

		/// <summary> UV offset for the dirt texture (if it's present) </summary>
		inline Vector2 DirtTextureOffset()const { return m_dirtOffset; }

		/// <summary>
		/// Applies offset to dirt texture (if it's present)
		/// </summary>
		/// <param name="offset"> Texture offset </param>
		inline void SetDirtTextureOffset(const Vector2& offset) { m_dirtOffset = offset; }

		/// <summary> If true, background color (like the skybox) will bloom too (disables depth-check) </summary>
		inline bool BloomBackground()const { return m_bloomBackground; }

		/// <summary>
		/// Sets 'BloomBackground' flag
		/// </summary>
		/// <param name="bloom"> If true, background color (like the skybox) will bloom too (disables depth-check) </param>
		inline void BloomBackground(bool bloom) { m_bloomBackground = bloom; }

		/// <summary> 
		/// Renderer category for render stack 
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererCategory()const;

		/// <summary>
		/// Sets renderer category for render stack
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="category"> Category to set </param>
		void SetRendererCategory(uint32_t category);

		/// <summary> 
		/// Renderer priority for render stack 
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererPriority()const;

		/// <summary>
		/// Sets renderer priority for render stack
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="priority"> Priority to set </param>
		void SetRendererPriority(uint32_t priority);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, whenever the component becomes recognized by engine </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, whenever the component gets destroyed </summary>
		virtual void OnComponentDestroyed()override;

	private:
		// Bloom filter "strength"
		float m_strength = 1.0f;

		// Bloom filter "spread"
		float m_size = 0.5f;

		// Bloom threshold
		float m_threshold = 0.8f;

		// Bloom threshold
		float m_thresholdSize = 0.1f;

		// Max color value
		float m_maxChannelIntensity = 1000000.0f;

		// Dirt texture
		Reference<Graphics::TextureSampler> m_dirtTexture = nullptr;
		float m_dirtStrength = 0.5f;
		Vector2 m_dirtTiling = Vector2(1.0f);
		Vector2 m_dirtOffset = Vector2(0.0f);

		// If true, background will bloom too (disables depth-check)
		bool m_bloomBackground = false;

		// Renderer category for render stack (higher category will render later)
		uint32_t m_category = 1024;

		// Renderer priority for render stack (higher priority will render earlier within the same category)
		uint32_t m_priority = 1024;

		// Render stack, this camera renders to
		Reference<RenderStack> m_renderStack;

		// Underlying RenderStack renderer
		Reference<RenderStack::Renderer> m_renderer;

		// Some private stuff resides here
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<BloomEffect>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<BloomEffect>(const Callback<const Object*>& report);
}
