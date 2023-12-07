#include "UIButton.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIButton::Helpers {
			static void RefreshTargetImageColor(UIButton* self, UIClickArea* area) {
				assert(area == self);
				Reference<UIImage> target = self->m_image;
				if (target == nullptr)
					return;
				const UIClickArea::StateFlags state = self->ClickState();
				target->SetColor(
					((state & UIClickArea::StateFlags::PRESSED) != UIClickArea::StateFlags::NONE) ? self->m_pressedColor :
					((state & UIClickArea::StateFlags::HOVERED) != UIClickArea::StateFlags::NONE) ? self->m_hoveredColor :
					self->m_idleColor);
			}
		};

		UIButton::UIButton(Component* parent, const std::string_view& name)
			: Component(parent, name), UIClickArea(parent, name) {
			const Callback<UIClickArea*> refreshCallback = Callback(&Helpers::RefreshTargetImageColor, this);
			OnFocusEnter() += refreshCallback;
			OnClicked() += refreshCallback;
			OnReleased() += refreshCallback;
			OnFocusExit() += refreshCallback;
		}

		UIButton::~UIButton() {
			const Callback<UIClickArea*> refreshCallback = Callback(&Helpers::RefreshTargetImageColor, this);
			OnFocusEnter() -= refreshCallback;
			OnClicked() -= refreshCallback;
			OnReleased() -= refreshCallback;
			OnFocusExit() -= refreshCallback;
		}

		void UIButton::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			UIClickArea::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ButtonImage, SetButtonImage, "Button Image", 
					"'Target' image the color of which will be set according to the button state");
				JIMARA_SERIALIZE_FIELD(m_idleColor, "Idle Color", 
					"Color when the button is neither hovered, not pressed", Object::Instantiate<Serialization::ColorAttribute>());
				JIMARA_SERIALIZE_FIELD(m_hoveredColor, "Hover Color", 
					"Color when the button is hovered, but not pressed", Object::Instantiate<Serialization::ColorAttribute>());
				JIMARA_SERIALIZE_FIELD(m_pressedColor, "Pressed Color", 
					"Color when the button is pressed", Object::Instantiate<Serialization::ColorAttribute>());
			};
			Helpers::RefreshTargetImageColor(this, this);
		}
	}

	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIButton>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIButton>(
			"UI Button", "Jimara/UI/Button", "UIClickArea that responds to hover and clicks by changing image color");
		report(factory);
	}
}
