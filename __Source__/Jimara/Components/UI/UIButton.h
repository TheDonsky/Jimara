#pragma once
#include "UIClickArea.h"
#include "UIImage.h"


namespace Jimara {
	namespace UI {
		/// <summary> Let the system know about the type </summary>
		JIMARA_REGISTER_TYPE(Jimara::UI::UIButton);

		/// <summary>
		/// UIClickArea that responds to hover and clicks by changing image color
		/// </summary>
		class JIMARA_API UIButton : public virtual UIClickArea {
		public:
			/// <summary> Button behaviour flags </summary>
			enum class JIMARA_API Flags : uint8_t {
				/// <summary> Empty bitmask </summary>
				NONE = 0u,

				/// <summary> If set, color values from settings will be applied (otherwise, they're ignored) </summary>
				APPLY_COLOR = 1u,

				/// <summary> If set, image texture will change based on the settings (otherwise, they're ignored) </summary>
				APPLY_TEXTURE = (1u << 1u),

				/// <summary> If set, OnButtonClicked() event will be fired when the ClickArea is released, instead of when it's clicked </summary>
				CLICK_ON_RELEASE = (1u << 2u),

				/// <summary> 
				/// If set, this flag forces the button to check and make sure the area is hovered before the OnButtonClicked() event is fired 
				/// (relevant only with CLICK_ON_RELEASE) 
				/// </summary>
				CHECK_HOVER_ON_CLICK = (1u << 3u),

				/// <summary> Default flags </summary>
				DEFAULT = APPLY_COLOR | APPLY_TEXTURE | CHECK_HOVER_ON_CLICK
			};

			/// <summary> Bitmask enumeration atrribute for Flags </summary>
			static const Object* FlagBitmaskAttribute();

			/// <summary> Button settings </summary>
			struct JIMARA_API Settings {
				/// <summary> Configuration flags </summary>
				Flags flags = Flags::DEFAULT;

				/// <summary> Color when the button is neither hovered, not pressed (ignored, unless glags contains APPLY_COLOR) </summary>
				Vector4 idleColor = Vector4(Vector3(0.8f), 1.0f);

				/// <summary> Texture when the button is neither hovered, not pressed (ignored, unless glags contains APPLY_TEXTURE) </summary>
				Reference<Graphics::TextureSampler> idleTexture = nullptr;

				/// <summary> Color when the button is hovered, but not pressed (ignored, unless glags contains APPLY_COLOR) </summary>
				Vector4 hoveredColor = Vector4(Vector3(1.0f), 1.0f);

				/// <summary> Texture when the button is hovered, but not pressed (ignored, unless glags contains APPLY_TEXTURE) </summary>
				Reference<Graphics::TextureSampler> hoveredTexture = nullptr;

				/// <summary> Color when the button is pressed (ignored, unless glags contains APPLY_COLOR) </summary>
				Vector4 pressedColor = Vector4(Vector3(0.5f), 1.0f);

				/// <summary> Texture when the button is pressed (ignored, unless glags contains APPLY_TEXTURE) </summary>
				Reference<Graphics::TextureSampler> pressedTexture = nullptr;
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Image name </param>
			UIButton(Component* parent, const std::string_view& name = "UIButton");

			/// <summary> Virtual destructor </summary>
			virtual ~UIButton();

			/// <summary> 'Target' image the color of which will be set according to the button state </summary>
			inline UIImage* ButtonImage()const { return m_image.operator Jimara::Reference<UIImage>(); }

			/// <summary> Button settings </summary>
			inline const Settings& ButtonSettings()const { return m_settings; }

			/// <summary> Button settings </summary>
			inline Settings& ButtonSettings() { return m_settings; }

			/// <summary> Invoked each time the button gets clicked (this is different from the clickable area clicks) </summary>
			inline Event<UIButton*>& OnButtonClicked() { return m_onButtonClicked; }

			/// <summary>
			/// Sets button target image
			/// </summary>
			/// <param name="image"> Image to be effected by click area events </param>
			inline void SetButtonImage(UIImage* image) { m_image = image; }

			/// <summary>
			/// Exposes fields to serialization utilities
			/// </summary>
			/// <param name="recordElement"> Reports elements with this </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		private:
			// 'Target' image the color of which will be set according to the button state
			WeakReference<UIImage> m_image;

			// Colors
			Settings m_settings;

			// Button click event
			EventInstance<UIButton*> m_onButtonClicked;

			// Private stuff resides in here...
			struct Helpers;
		};


		/// <summary>
		/// Bitwise inverse of UIButton::Flags
		/// </summary>
		/// <param name="f"> Flags </param>
		/// <returns> ~f </returns>
		inline constexpr UIButton::Flags operator~(UIButton::Flags f) {
			return static_cast<UIButton::Flags>(~static_cast<std::underlying_type_t<UIButton::Flags>>(f));
		}

		/// <summary>
		/// Logical 'or' between two UIButton::Flags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a | b </returns>
		inline constexpr UIButton::Flags operator|(UIButton::Flags a, UIButton::Flags b) {
			return static_cast<UIButton::Flags>(
				static_cast<std::underlying_type_t<UIButton::Flags>>(a) | static_cast<std::underlying_type_t<UIButton::Flags>>(b));
		}

		/// <summary>
		/// Logical 'or' between two UIButton::Flags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a </returns>
		inline static UIButton::Flags& operator|=(UIButton::Flags& a, UIButton::Flags b) { return a = (a | b); }

		/// <summary>
		/// Logical 'and' between two UIButton::Flags bitmasks
		/// </summary>
		/// <param name="a"> First mask </param>
		/// <param name="b"> Second mask </param>
		/// <returns> a & b </returns>
		inline constexpr UIButton::Flags operator&(UIButton::Flags a, UIButton::Flags b) {
			return static_cast<UIButton::Flags>(
				static_cast<std::underlying_type_t<UIButton::Flags>>(a) & static_cast<std::underlying_type_t<UIButton::Flags>>(b));
		}
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIButton>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIButton>(const Callback<const Object*>& report);
}

