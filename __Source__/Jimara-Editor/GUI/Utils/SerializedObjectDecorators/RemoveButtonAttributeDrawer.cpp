#include "RemoveButtonAttributeDrawer.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <Data/Serialization/Attributes/RemoveButtonAttribute.h>


namespace Jimara {
	namespace Editor {
		RemoveButtonAttributeDrawer::RemoveButtonAttributeDrawer() 
			: SerializedObjectDecoratorDrawer(TypeId::Of<Serialization::RemoveButtonAttribute>()) {};

		bool RemoveButtonAttributeDrawer::DecorateObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger, const Object* attribute)const {
			const Serialization::RemoveButtonAttribute* removeButtonAttr = dynamic_cast<const Serialization::RemoveButtonAttribute*>(attribute);
			if (removeButtonAttr == nullptr) {
				if (logger != nullptr) logger->Error("RemoveButtonAttributeDrawer::DecorateObject - Invalid attribute provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			ImGui::SameLine();
			const std::string text = [&]() {
				std::stringstream stream;
				stream << ICON_FA_MINUS_CIRCLE << "###RemoveButtonAttributeDrawer_" << (viewId)
					<< "_delete_btn_" << ((size_t)object.TargetAddr()) << "_" << ((size_t)object.Serializer());
				return stream.str();
			}();
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
			bool rv = ImGui::Button(text.c_str());
			if (rv) 
				removeButtonAttr->OnButtonClicked(object);
			ImGui::PopStyleColor();

			return rv;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::RemoveButtonAttributeDrawer>(const Callback<const Object*>& report) {
		static const Reference<const Editor::RemoveButtonAttributeDrawer> drawer = Object::Instantiate<Editor::RemoveButtonAttributeDrawer>();
		report(drawer);
	}
}
