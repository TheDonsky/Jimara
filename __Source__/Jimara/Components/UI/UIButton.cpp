#include "UIButton.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIButton::Helpers {
			static void RefreshTargetImageAppearance(UIButton* self, UIClickArea* area) {
				assert(area == self);
				Reference<UIImage> target = self->m_image;
				if (target == nullptr)
					return;
				const UIClickArea::StateFlags state = self->ClickState();
				if ((self->m_settings.flags & Flags::APPLY_COLOR) != Flags::NONE)
					target->SetColor(
						((state & UIClickArea::StateFlags::PRESSED) != UIClickArea::StateFlags::NONE) ? self->m_settings.pressedColor :
						((state & UIClickArea::StateFlags::HOVERED) != UIClickArea::StateFlags::NONE) ? self->m_settings.hoveredColor :
						self->m_settings.idleColor);
				if ((self->m_settings.flags & Flags::APPLY_TEXTURE) != Flags::NONE)
					target->SetTexture(
						((state & UIClickArea::StateFlags::PRESSED) != UIClickArea::StateFlags::NONE) ? self->m_settings.pressedTexture :
						((state & UIClickArea::StateFlags::HOVERED) != UIClickArea::StateFlags::NONE) ? self->m_settings.hoveredTexture :
						self->m_settings.idleTexture);
			}

			static void OnAreaActionPerformed(UIButton* self, UIClickArea* area) {
				assert(area == self);
				RefreshTargetImageAppearance(self, area);
				const UIClickArea::StateFlags state = self->ClickState();
				if ((self->m_settings.flags & Flags::CHECK_HOVER_ON_CLICK) != Flags::NONE &&
					(state & UIClickArea::StateFlags::HOVERED) == UIClickArea::StateFlags::NONE)
					return;
				const UIClickArea::StateFlags clickFlag =
					((self->m_settings.flags & Flags::CLICK_ON_RELEASE) != Flags::NONE)
					? UIClickArea::StateFlags::GOT_RELEASED : UIClickArea::StateFlags::GOT_PRESSED;
				if ((state & clickFlag) != UIClickArea::StateFlags::NONE)
					self->m_onButtonClicked(self);
			}
		};

		const Object* UIButton::FlagBitmaskAttribute() {
			static const Reference<const Object> attribute =
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Flags>>>(true,
					"NONE", Flags::NONE,
					"APPLY_COLOR", Flags::APPLY_COLOR,
					"APPLY_TEXTURE", Flags::APPLY_TEXTURE,
					"CLICK_ON_RELEASE", Flags::CLICK_ON_RELEASE,
					"CHECK_HOVER_ON_CLICK", Flags::CHECK_HOVER_ON_CLICK);
			return attribute;
		}

		UIButton::UIButton(Component* parent, const std::string_view& name)
			: Component(parent, name), UIClickArea(parent, name) {
			const Callback<UIClickArea*> refreshCallback = Callback(&Helpers::RefreshTargetImageAppearance, this);
			const Callback<UIClickArea*> clickCallback = Callback(&Helpers::OnAreaActionPerformed, this);
			OnFocusEnter() += refreshCallback;
			OnClicked() += clickCallback;
			OnReleased() += clickCallback;
			OnFocusExit() += refreshCallback;
		}

		UIButton::~UIButton() {
			const Callback<UIClickArea*> refreshCallback = Callback(&Helpers::RefreshTargetImageAppearance, this);
			const Callback<UIClickArea*> clickCallback = Callback(&Helpers::OnAreaActionPerformed, this);
			OnFocusEnter() -= refreshCallback;
			OnClicked() -= clickCallback;
			OnReleased() -= clickCallback;
			OnFocusExit() -= refreshCallback;
		}

		void UIButton::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			UIClickArea::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ButtonImage, SetButtonImage, "Button Image", 
					"'Target' image the color of which will be set according to the button state");
				
				JIMARA_SERIALIZE_FIELD(m_settings.flags, "Button Flags", "Button settings", FlagBitmaskAttribute());
				
				if ((m_settings.flags & Flags::APPLY_COLOR) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.idleColor, "Idle Color",
						"Color when the button is neither hovered, not pressed", Object::Instantiate<Serialization::ColorAttribute>());
				if ((m_settings.flags & Flags::APPLY_TEXTURE) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.idleTexture, "Idle Texture", "Texture when the button is neither hovered, not pressed");

				if ((m_settings.flags & Flags::APPLY_COLOR) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.hoveredColor, "Hover Color",
						"Color when the button is hovered, but not pressed", Object::Instantiate<Serialization::ColorAttribute>());
				if ((m_settings.flags & Flags::APPLY_TEXTURE) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.hoveredTexture, "Hover Texture", "Texture when the button is hovered, but not pressed");

				if ((m_settings.flags & Flags::APPLY_COLOR) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.pressedColor, "Pressed Color",
						"Color when the button is pressed", Object::Instantiate<Serialization::ColorAttribute>());
				if ((m_settings.flags & Flags::APPLY_TEXTURE) != Flags::NONE)
					JIMARA_SERIALIZE_FIELD(m_settings.pressedTexture, "Idle Texture", "Texture when the button is pressed");
			};
			Helpers::RefreshTargetImageAppearance(this, this);
		}
	}

	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIButton>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIButton>(
			"UI Button", "Jimara/UI/Button", "UIClickArea that responds to hover and clicks by changing image color");
		report(factory);
	}
}
