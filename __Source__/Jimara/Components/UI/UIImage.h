#pragma once
#include "UITransform.h"


namespace Jimara {
	namespace UI {
		/// <summary> Let the system know of the type </summary>
		JIMARA_REGISTER_TYPE(Jimara::UI::UIImage);

		/// <summary>
		/// Image that can appear on UI Canvas
		/// </summary>
		class JIMARA_API UIImage : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Image name </param>
			UIImage(Component* parent, const std::string_view& name = "UIImage");

			/// <summary> Virtual destructor </summary>
			virtual ~UIImage();


			/// <summary> Image's Texture field will override a shader input of this name </summary>
			inline static const constexpr std::string_view TextureShaderBindingName() { return "mainTexture"; };

			/// <summary> Sampler to the main texture (overrides material filed with the name: TextureShaderBindingName()) </summary>
			inline Graphics::TextureSampler* Texture()const { return m_texture; }

			/// <summary>
			/// Sets main texture sampler
			/// </summary>
			/// <param name="texture"> Sampler to the main texture </param>
			inline void SetTexture(Graphics::TextureSampler* texture) { m_texture = texture; }


			/// <summary> Image's Color field will override a shader instance input of this name </summary>
			inline static const constexpr std::string_view ColorShaderBindingName() { return "VertexColor"; };

			/// <summary> Image color multiplier (appears as vertex color input with the name: ColorShaderBindingName()) </summary>
			inline Vector4 Color()const { return m_color; }

			/// <summary>
			/// Sets image color
			/// </summary>
			/// <param name="color"> Instance color of the image </param>
			inline void SetColor(const Vector4& color) { m_color = color; }


			/// <summary> If true, the UIImage will keep the aspect ratio of the Texture </summary>
			inline bool KeepAspectRatio()const { return m_keepAspectRatio; }

			/// <summary>
			/// Configures, whether or not to keep the main texture aspect ratio
			/// </summary>
			/// <param name="preserve"> If true, Texture aspect ratio will be preserved </param>
			inline void KeepAspectRatio(bool preserve) { m_keepAspectRatio = preserve; }

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

			/// <summary> Invoked, whenever parent hierarchy gets dirty </summary>
			virtual void OnParentChainDirty()override;


		private:
			// Texture
			Reference<Graphics::TextureSampler> m_texture;

			// Color
			Vector4 m_color = Vector4(1.0f);

			// Aspect ratio flag
			bool m_keepAspectRatio = true;

			// Material
			Reference<Material> m_material;
			Reference<Material::Instance> m_materialInstance;

			// Underlying graphics object and Canvas
			Reference<const Canvas> m_canvas;
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_graphicsObject;
			
			// Private stuff resides in here..
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIImage>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIImage>(const Callback<const Object*>& report);
}
