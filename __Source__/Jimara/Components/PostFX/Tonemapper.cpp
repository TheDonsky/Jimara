#include "Tonemapper.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Environment/Rendering/PostFX/Tonemapper/TonemapperKernel.h"


namespace Jimara {
	struct Tonemapper::Helpers {
		class Renderer
			: public virtual RenderStack::Renderer
			, public virtual JobSystem::Job {
		private:
			const Tonemapper* const m_owner;
			const Reference<SceneContext> m_context;
			const Reference<TonemapperKernel> m_kernel;
			Reference<RenderImages> m_renderImages;

		public:
			inline Renderer(const Tonemapper* owner)
				: m_owner(owner)
				, m_context(owner->Context())
				, m_kernel(TonemapperKernel::Create(
					owner->Context()->Graphics()->Device(),
					owner->Context()->Graphics()->Configuration().ShaderLoader(),
					owner->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount())) {
				if (m_kernel == nullptr)
					m_context->Log()->Error("Tonemapper::Helpers::Renderer - Failed to create TonemapperKernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline virtual ~Renderer() {}

			inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) override {
				if (m_kernel == nullptr) return;
				if (m_renderImages != images) {
					m_renderImages = images;
					RenderImages::Image* image = images->GetImage(RenderImages::MainColor());
					if (image == nullptr) {
						m_context->Log()->Error("Tonemapper::Helpers::Renderer::Render - Failed to retrieve main image! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
					m_kernel->SetTarget(image->Resolve());
				}
				m_kernel->Execute(commandBufferInfo);
			}

			inline virtual void Execute() {
				// Once we add more parameters, we'll expose them from here...
			}
			inline virtual void CollectDependencies(Callback<Job*>) {}
		};

		inline static void RemoveRenderer(Tonemapper* self) {
			if (self->m_renderer == nullptr) return;
			if (self->m_renderStack == nullptr) {
				self->Context()->Log()->Error("Tonemapper::Helpers::RemoveRenderer - [Internal Error] Render stack missing! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			self->Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(self->m_renderer.operator->()));
			self->m_renderStack->RemoveRenderer(self->m_renderer);
			self->m_renderer = nullptr;
		}

		inline static void AddRenderer(Tonemapper* self) {
			RemoveRenderer(self);
			if (self->m_renderStack == nullptr)
				self->m_renderStack = RenderStack::Main(self->Context());
			if (self->m_renderStack == nullptr) {
				self->Context()->Log()->Error("Tonemapper::Helpers::AddRenderer - Render stack could not be retrieved! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			Reference<Renderer> renderer = Object::Instantiate<Renderer>(self);
			self->m_renderer = renderer;
			renderer->SetCategory(self->RendererCategory());
			renderer->SetPriority(self->RendererPriority());

			self->m_renderStack->AddRenderer(self->m_renderer);
			self->Context()->Graphics()->SynchPointJobs().Add(renderer);
		}

		inline static void ManageRenderer(Tonemapper* self) {
			if (self->Destroyed() || (!self->ActiveInHeirarchy()))
				RemoveRenderer(self);
			else if (self->m_renderer == nullptr)
				AddRenderer(self);
		}
	};

	Tonemapper::Tonemapper(Component* parent, const std::string_view& name) 
		: Component(parent, name) {}

	Tonemapper::~Tonemapper() {
		Helpers::RemoveRenderer(this);
	}

	uint32_t Tonemapper::RendererCategory()const { return m_category; }

	void Tonemapper::SetRendererCategory(uint32_t category) {
		m_category = category;
		if (m_renderer != nullptr)
			m_renderer->SetCategory(m_category);
	}

	uint32_t Tonemapper::RendererPriority()const { return m_priority; }

	void Tonemapper::SetRendererPriority(uint32_t priority) {
		m_priority = priority;
		if (m_renderer != nullptr)
			m_renderer->SetPriority(m_priority);
	}

	void Tonemapper::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(
				RendererCategory, SetRendererCategory,
				"Render Category", "Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.");
			JIMARA_SERIALIZE_FIELD_GET_SET(
				RendererPriority, SetRendererPriority,
				"Render Priority", "Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.");
		};
	}

	void Tonemapper::OnComponentInitialized() {
		Helpers::ManageRenderer(this);
	}

	void Tonemapper::OnComponentEnabled() {
		Helpers::ManageRenderer(this);
	}

	void Tonemapper::OnComponentDisabled() {
		Helpers::ManageRenderer(this);
	}

	void Tonemapper::OnComponentDestroyed() {
		Helpers::RemoveRenderer(this);
		m_renderStack = nullptr;
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Tonemapper>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<Tonemapper> serializer("Jimara/PostFX/Tonemapper", "Tonemapper");
		report(&serializer);
	}
}
