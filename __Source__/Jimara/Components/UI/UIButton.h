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
			Vector4 m_idleColor = Vector4(Vector3(0.8f), 1.0f);
			Vector4 m_hoveredColor = Vector4(Vector3(1.0f), 1.0f);
			Vector4 m_pressedColor = Vector4(Vector3(0.5f), 1.0f);

			// Private stuff resides in here...
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIButton>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIButton>(const Callback<const Object*>& report);
}

