#include "UIText.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIText::Helpers {

			inline static void RefreshGraphicsObject(UIText* self) {
				if (self->m_graphicsObject != nullptr) {
					self->m_canvas->GraphicsObjects()->Remove(self->m_graphicsObject);
					self->Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(self->m_graphicsObject->Item()));
					self->m_graphicsObject = nullptr;
				}
				self->m_canvas = nullptr;

				if (!self->ActiveInHeirarchy()) return;

				self->m_canvas = self->GetComponentInParents<Canvas>();
				if (self->m_canvas == nullptr) return;

				// __TODO__: Implement this crap!
				const Reference<GraphicsObjectDescriptor> graphicsObject = nullptr;// GraphicsObject::Create(self);
				if (graphicsObject == nullptr) {
					self->m_canvas = nullptr;
					return;
				}
				self->m_graphicsObject = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(graphicsObject);

				self->Context()->Graphics()->SynchPointJobs().Add(dynamic_cast<JobSystem::Job*>(self->m_graphicsObject->Item()));
				self->m_canvas->GraphicsObjects()->Add(self->m_graphicsObject);
			}

			inline static void UnsubscribeParentChain(UIText* self) {
				for (size_t i = 0; i < self->m_parentChain.Size(); i++)
					self->m_parentChain[i]->OnParentChanged() -= Callback<ParentChangeInfo>(Helpers::OnParentChanged, self);
				self->m_parentChain.Clear();
			}

			inline static void SubscribeParentChain(UIText* self) {
				UnsubscribeParentChain(self);
				if (self->Destroyed()) return;
				Component* parent = self;
				while (parent != nullptr) {
					parent->OnParentChanged() += Callback<ParentChangeInfo>(Helpers::OnParentChanged, self);
					self->m_parentChain.Push(parent);
					if (dynamic_cast<Canvas*>(parent) != nullptr) break;
					else parent = parent->Parent();
				}
			}

			inline static void OnParentChanged(UIText* self, ParentChangeInfo) {
				RefreshGraphicsObject(self);
				SubscribeParentChain(self);
			}

			inline static void OnImageDestroyed(Component* self) {
				Helpers::UnsubscribeParentChain(dynamic_cast<UIText*>(self));
				Helpers::RefreshGraphicsObject(dynamic_cast<UIText*>(self));
				self->OnDestroyed() -= Callback(Helpers::OnImageDestroyed);
			}
		};

		UIText::UIText(Component* parent, const std::string_view& name) 
			: Component(parent, name) {
			Helpers::SubscribeParentChain(this);
			OnDestroyed() += Callback(Helpers::OnImageDestroyed);
		}

		UIText::~UIText() {
			Helpers::OnImageDestroyed(this);
		}

		void UIText::SetFont(Jimara::Font* font) {
			if (m_font == font)
				return;
			m_font = font;
			Helpers::RefreshGraphicsObject(this);
		}

		void UIText::SetFontSize(float size) {
			m_fontSize = size;
		}

		void UIText::SetColor(const Vector4& color) {
			m_color = color;
		}

		void UIText::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD(m_text, "Text", "Displayed text");

				static const std::string fontHint = "Sampler to the main texture (overrides material filed named '" + std::string(FontTextureShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Font, SetFont, "Font", fontHint);
				JIMARA_SERIALIZE_FIELD_GET_SET(FontSize, SetFontSize, "Font Size", "Font size in canvas units");

				static const std::string colorHint = "Image color multiplier (appears as vertex color input with the name: '" + std::string(ColorShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Color, SetColor, "Color", colorHint, Object::Instantiate<Serialization::ColorAttribute>());
			};
		}

		void UIText::OnComponentEnabled() {
			Helpers::RefreshGraphicsObject(this);
		}

		void UIText::OnComponentDisabled() {
			Helpers::RefreshGraphicsObject(this);
		}
	}

	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIText>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIText>(
			"UI Image", "Jimara/UI/Text", "Text that can appear on UI Canvas");
		report(factory);
	}
}
