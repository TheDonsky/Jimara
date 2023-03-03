#include "Canvas.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"
#include "../../Environment/Rendering/LightingModels/UnlitRendering/CanvasRenderer.h"


namespace Jimara {
	namespace UI {
		struct Canvas::Helpers {
			static void OnCanvasDestroyed(Canvas* self, Component*) {
				self->OnDestroyed() -= Callback<Component*>(Helpers::OnCanvasDestroyed, self);
				if (self->m_renderStack != nullptr && self->m_renderer != nullptr)
						self->m_renderStack->RemoveRenderer(self->m_renderer);
				self->m_renderer = nullptr;
				self->m_renderStack = nullptr;
				self->m_graphicsObjects = nullptr;
			}

			static void OnEnabledOrDisabled(Canvas* self) {
				if (self->m_renderStack == nullptr || self->m_renderer == nullptr) return;
				if (self->ActiveInHeirarchy())
					self->m_renderStack->AddRenderer(self->m_renderer);
				else self->m_renderStack->RemoveRenderer(self->m_renderer);
			}
		};

		Canvas::Canvas(Component* parent, const std::string_view& name) 
			: Component(parent, name) {
			m_graphicsObjects = Object::Instantiate<GraphicsObjectDescriptor::Set>(Context());
			m_renderStack = RenderStack::Main(Context());
			m_renderer = CanvasRenderer::CreateFor(this);
			OnDestroyed() += Callback<Component*>(Helpers::OnCanvasDestroyed, this);
		}

		Canvas::~Canvas() {
			Helpers::OnCanvasDestroyed(this, nullptr);
		}

		Vector2 Canvas::Size()const {
			const auto aspect = [](const Vector2& size) { return size.x / Math::Max(size.y, std::numeric_limits<float>::epsilon()); };
			const Vector2 textureSize = m_renderStack == nullptr ? Vector2(0.0f) : (Vector2)m_renderStack->Resolution();
			const float targetAspect = aspect(textureSize);
			const float referenceAspect = aspect(m_referenceResolution);
			const float rescaledX = m_referenceResolution.x * targetAspect / Math::Max(referenceAspect, std::numeric_limits<float>::epsilon());
			const float rescaledY = m_referenceResolution.y * referenceAspect / Math::Max(targetAspect, std::numeric_limits<float>::epsilon());
			return Vector2(
				Math::Lerp(rescaledX, m_referenceResolution.x, m_widthBias),
				Math::Lerp(m_referenceResolution.y, rescaledY, m_widthBias));
		}

		void Canvas::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ReferenceSize, SetReferenceSize, "Reference Size", 
					"Reference 'resolution'/virtual size of the canvas");
				
				JIMARA_SERIALIZE_FIELD_GET_SET(WidthBias, SetWidthBias, "Width Bias",
					"Value between 0 and 1, indicating whether the virtual size of the canvas scales horizontally(0.0f) or vertically(1.0f)",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				
				JIMARA_SERIALIZE_FIELD_GET_SET(RendererCategory, SetRendererCategory, "Render Category", 
					"Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.");
				
				JIMARA_SERIALIZE_FIELD_GET_SET(RendererPriority, SetRendererPriority, "Render Priority", 
					"Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.");
			};
		}

		void Canvas::OnComponentEnabled() {
			Helpers::OnEnabledOrDisabled(this);
		}

		void Canvas::OnComponentDisabled() {
			Helpers::OnEnabledOrDisabled(this);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<UI::Canvas>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<UI::Canvas> serializer("Jimara/UI/Canvas", "Canvas");
		report(&serializer);
	}
}
