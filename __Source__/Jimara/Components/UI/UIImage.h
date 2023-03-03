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
			inline static const constexpr std::string_view TextureShaderBindingName() { return "MainTexture"; };

			/// <summary> Sampler to the main texture (overrides material filed with the name: TextureShaderBindingName()) </summary>
			Graphics::TextureSampler* Texture()const;

			/// <summary>
			/// Sets main texture sampler
			/// </summary>
			/// <param name="SetTexture"> Sampler to the main texture </param>
			void SetTexture(Graphics::TextureSampler* SetTexture);

			/// <summary> If true, the UIImage will keep the aspect ratio of the Texture </summary>
			bool KeepAspectRatio()const;

			/// <summary>
			/// Configures, whether or not to keep the main texture aspect ratio
			/// </summary>
			/// <param name="preserve"> If true, Texture aspect ratio will be preserved </param>
			void KeepAspectRatio(bool preserve);

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

			// Private stuff resides in here..
			struct Helpers;
		};
	}

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<UI::UIImage>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIImage>(const Callback<const Object*>& report);
}
