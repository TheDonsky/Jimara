#include "UIImage.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	namespace UI {
		struct UIImage::Helpers {
			// __TODO__: Implement this crap!
		};


		UIImage::UIImage(Component* parent, const std::string_view& name) 
			: Component(parent, name) {
			// __TODO__: Implement this crap!
		}

		UIImage::~UIImage() {
			// __TODO__: Implement this crap!
		}

		Graphics::TextureSampler* UIImage::Texture()const {
			// __TODO__: Implement this crap!
			return nullptr;
		}

		void UIImage::SetTexture(Graphics::TextureSampler* SetTexture) {
			// __TODO__: Implement this crap!
		}

		bool UIImage::KeepAspectRatio()const {
			// __TODO__: Implement this crap!
			return false;
		}

		void UIImage::KeepAspectRatio(bool preserve) {
			// __TODO__: Implement this crap!
		}

		void UIImage::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				static const std::string textureHint = "Sampler to the main texture (overrides material filed named '" + std::string(TextureShaderBindingName()) + "')";
				JIMARA_SERIALIZE_FIELD_GET_SET(Texture, SetTexture, "Texture", textureHint);
				JIMARA_SERIALIZE_FIELD_GET_SET(KeepAspectRatio, KeepAspectRatio, "Keep Aspect", "If true, the UIImage will keep the aspect ratio of the Texture");
			};
		}

		void UIImage::OnComponentEnabled() {
			// __TODO__: Implement this crap!
		}

		void UIImage::OnComponentDisabled() {
			// __TODO__: Implement this crap!
		}
	}


	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIImage>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<UI::UIImage> serializer("Jimara/UI/Image", "Image");
		report(&serializer);
	}
}
