#pragma once
#include "UITransform.h"
#include "../../Data/Fonts/Font.h"


namespace Jimara {
	namespace UI {
		/// <summary> Let the system know of the type </summary>
		JIMARA_REGISTER_TYPE(Jimara::UI::UIText);

		/// <summary>
		/// Text, that can appear on UI Canvas
		/// </summary>
		class JIMARA_API UIText : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Image name </param>
			UIText(Component* parent, const std::string_view& name = "UIText");

			/// <summary> Virtual destructor </summary>
			virtual ~UIText();

			/// <summary> Image's Texture field will override a shader input of this name </summary>
			inline static const constexpr std::string_view FontTextureShaderBindingName() { return "fontAlpha"; };

			/// <summary> Displayed text </summary>
			inline std::string& Text() { return m_text; }

			/// <summary> Displayed text </summary>
			inline const std::string& Text()const { return m_text; }

			/// <summary> Font for the text </summary>
			inline Jimara::Font* Font()const { return m_font; }

			/// <summary>
			/// Sets font
			/// </summary>
			/// <param name="font"> Font to use </param>
			void SetFont(Jimara::Font* font);

			/// <summary> Font size in canvas units </summary>
			inline float FontSize()const { return m_fontSize; }

			/// <summary>
			/// Sets character size
			/// </summary>
			/// <param name="size"> Font size in canvas units </param>
			void SetFontSize(float size);

			/// <summary> Image's Color field will override a shader instance input of this name </summary>
			inline static const constexpr std::string_view ColorShaderBindingName() { return "VertexColor"; };

			/// <summary> Image color multiplier (appears as vertex color input with the name: ColorShaderBindingName()) </summary>
			inline Vector4 Color()const { return m_color; }

			/// <summary>
			/// Sets image color
			/// </summary>
			/// <param name="color"> Instance color of the image </param>
			void SetColor(const Vector4& color);

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
			// Displayed text
			std::string m_text;

			// Font
			Reference<Jimara::Font> m_font;

			// Font size
			float m_fontSize = 24.0f;

			// Color
			Vector4 m_color = Vector4(1.0f);

			// Material
			Reference<Material> m_material;
			Reference<Material::Instance> m_materialInstance;

			// Parent object chain (for listening to hierarchical changes)
			Stacktor<Reference<Component>, 4u> m_parentChain;

			// Underlying graphics object and Canvas
			Reference<const Canvas> m_canvas;
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_graphicsObject;

			// Private stuff resides in here..
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIText>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIText>(const Callback<const Object*>& report);
}
