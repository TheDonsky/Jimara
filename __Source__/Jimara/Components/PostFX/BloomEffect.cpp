#include "BloomEffect.h"
#include "../../Environment/Rendering/PostFX/Bloom/BloomKernel.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Data/Serialization/Attributes/DragSpeedAttribute.h"


namespace Jimara {
	struct BloomEffect::Helpers {
		class Renderer
			: public virtual RenderStack::Renderer
			, public virtual JobSystem::Job {
		private:
			const BloomEffect* const m_owner;
			const Reference<SceneContext> m_context;
			const Reference<BloomKernel> m_bloomKernel;
			bool m_bloomBackground = false;
			Reference<RenderImages> m_renderImages;
			Reference<Graphics::TextureSampler> m_colorSampler;
			Reference<Graphics::TextureSampler> m_depthSampler;

		public:
			inline Renderer(const BloomEffect* owner)
				: m_owner(owner)
				, m_context(owner->Context())
				, m_bloomKernel(BloomKernel::Create(
					owner->Context()->Graphics()->Device(),
					owner->Context()->Graphics()->Configuration().ShaderLibrary(),
					owner->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount())) {
				if (m_bloomKernel == nullptr)
					m_context->Log()->Error("BloomEffect::Helpers::Renderer - Failed to create BloomKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline virtual ~Renderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
				if (m_bloomKernel == nullptr) return;
				if (m_renderImages != images) {
					m_renderImages = images;
					m_colorSampler = nullptr;
					m_depthSampler = nullptr;
					auto createSampler = [&](const RenderImages::ImageId* imageId) -> Reference<Graphics::TextureSampler> {
						auto fail = [&](const auto&... message) -> Reference<Graphics::TextureSampler> {
							m_context->Log()->Error("BloomEffect::Helpers::Renderer::Render - ", message...);
							return nullptr;
						};
						RenderImages::Image* image = images->GetImage(imageId);
						if (image == nullptr)
							return fail("Failed to retrieve image![File:", __FILE__, "; Line: ", __LINE__, "]");
						Graphics::TextureView* view = image->Resolve();
						if (view == nullptr)
							return fail("Failed to retrieve image view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						const Reference<Graphics::TextureSampler> sampler = view->CreateSampler(
							Graphics::TextureSampler::FilteringMode::LINEAR, Graphics::TextureSampler::WrappingMode::CLAMP_TO_BORDER);
						if (sampler == nullptr)
							return fail("Failed to create image sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return sampler;
					};
					if (images != nullptr) {
						m_colorSampler = createSampler(RenderImages::MainColor());
						m_depthSampler = createSampler(RenderImages::DepthBuffer());
					}
					m_bloomKernel->SetTarget(m_colorSampler, m_bloomBackground ? nullptr : m_depthSampler.operator->());
				}
				m_bloomKernel->Execute(commandBufferInfo);
			}

			inline virtual void Execute() {
				if (m_bloomKernel == nullptr) return;
				if (m_bloomBackground != m_owner->BloomBackground()) {
					m_renderImages = nullptr;
					m_bloomBackground = m_owner->BloomBackground();
				}
				m_bloomKernel->Configure(m_owner->Strength(), m_owner->Size(), m_owner->Threshold(), m_owner->ThresholdSize(), m_owner->MaxChannelIntensity());
				m_bloomKernel->SetDirtTexture(m_owner->DirtTexture(), m_owner->DirtStrength(), m_owner->DirtTextureTiling(), m_owner->DirtTextureOffset());
			}
			inline virtual void CollectDependencies(Callback<Job*>) {}
		};

		inline static void RemoveRenderer(BloomEffect* self) {
			if (self->m_renderer == nullptr) return;
			if (self->m_renderStack == nullptr) {
				self->Context()->Log()->Error("BloomEffect::Helpers::RemoveRenderer - [Internal Error] Render stack missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			self->Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(self->m_renderer.operator->()));
			self->m_renderStack->RemoveRenderer(self->m_renderer);
			self->m_renderer = nullptr;
		}

		inline static void AddRenderer(BloomEffect* self) {
			RemoveRenderer(self);
			if (self->m_renderStack == nullptr)
				self->m_renderStack = RenderStack::Main(self->Context());
			if (self->m_renderStack == nullptr) {
				self->Context()->Log()->Error("BloomEffect::Helpers::AddRenderer - Render stack could not be retrieved! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			Reference<Renderer> renderer = Object::Instantiate<Renderer>(self);
			self->m_renderer = renderer;
			renderer->SetCategory(self->RendererCategory());
			renderer->SetPriority(self->RendererPriority());

			self->m_renderStack->AddRenderer(self->m_renderer);
			self->Context()->Graphics()->SynchPointJobs().Add(renderer);
		}
		
		inline static void ManageRenderer(BloomEffect* self) {
			if (self->Destroyed() || (!self->ActiveInHierarchy()))
				RemoveRenderer(self);
			else if (self->m_renderer == nullptr)
				AddRenderer(self);
		}
	};

	BloomEffect::BloomEffect(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	BloomEffect::~BloomEffect() {
		Helpers::RemoveRenderer(this);
	}

	uint32_t BloomEffect::RendererCategory()const { return m_category; }

	void BloomEffect::SetRendererCategory(uint32_t category) {
		m_category = category;
		if (m_renderer != nullptr)
			m_renderer->SetCategory(m_category);
	}

	uint32_t BloomEffect::RendererPriority()const { return m_priority; }

	void BloomEffect::SetRendererPriority(uint32_t priority) {
		m_priority = priority;
		if (m_renderer != nullptr)
			m_renderer->SetPriority(m_priority);
	}

	void BloomEffect::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(
				Strength, SetStrength,
				"Strength", "Amount of bloom applied to final image",
				Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			JIMARA_SERIALIZE_FIELD_GET_SET(
				Size, SetSize, "Size", "Size of bloom effect",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
			JIMARA_SERIALIZE_FIELD_GET_SET(
				Threshold, SetThreshold,
				"Threshold", "Minimal pixel intensity for it to start \"blooming\" (negative values mean 'no thresholding')",
				Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			JIMARA_SERIALIZE_FIELD_GET_SET(
				ThresholdSize, SetThresholdSize,
				"Threshold Size", "Bloom will gradually fade in and out between intensities equal to threshold and (threshold + thresholdSize)",
				Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			JIMARA_SERIALIZE_FIELD_GET_SET(
				MaxChannelIntensity, SetMaxChannelIntensity,
				"Max Channel Intensity", "Input color channel values will be clamped to this to avoid 'exploding-infinite intencity pixels' from ruining the image");
			JIMARA_SERIALIZE_FIELD_GET_SET(
				DirtTexture, SetDirtTexture,
				"Dirt Texture", "Dirt texture, that will show up as an overly on bloomed areas (optional)");
			if (DirtTexture() != nullptr) {
				JIMARA_SERIALIZE_FIELD_GET_SET(
					DirtStrength, SetDirtStrength,
					"Dirt Strength", "Dirt texture intensity",
					Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
				JIMARA_SERIALIZE_FIELD_GET_SET(
					DirtTextureTiling, SetDirtTextureTiling,
					"Dirt Texture Tiling", "Tiling for the dirt texture",
					Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
				JIMARA_SERIALIZE_FIELD_GET_SET(
					DirtTextureOffset, SetDirtTextureOffset,
					"Dirt Texture Offset", "UV offset for the dirt texture",
					Object::Instantiate<Serialization::DragSpeedAttribute>(0.01f));
			}

			JIMARA_SERIALIZE_FIELD_GET_SET(BloomBackground, BloomBackground, "Bloom Background",
				"If true, background color (like the skybox) will bloom too (disables depth-check)");
			
			JIMARA_SERIALIZE_FIELD_GET_SET(
				RendererCategory, SetRendererCategory, 
				"Render Category", "Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.");
			JIMARA_SERIALIZE_FIELD_GET_SET(
				RendererPriority, SetRendererPriority,
				"Render Priority", "Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.");
		};
	}

	void BloomEffect::OnComponentInitialized() {
		Helpers::ManageRenderer(this);
	}

	void BloomEffect::OnComponentEnabled() {
		Helpers::ManageRenderer(this);
	}

	void BloomEffect::OnComponentDisabled() {
		Helpers::ManageRenderer(this);
	}

	void BloomEffect::OnComponentDestroyed() {
		Helpers::RemoveRenderer(this);
		m_renderStack = nullptr;
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<BloomEffect>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<BloomEffect>(
			"Bloom", "Jimara/PostFX/BloomEffect", "Bloom post process effect");
		report(factory);
	}
}
