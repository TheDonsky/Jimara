#include "TextBoxAttributeDrawer.h"
#include "../DrawTooltip.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static const TextBoxAttributeDrawer* MainTextBoxAttributeDrawer() {
				static const Reference<const TextBoxAttributeDrawer> drawer = Object::Instantiate<TextBoxAttributeDrawer>();
				return drawer;
			}
			inline static const constexpr Serialization::SerializerTypeMask TextBoxAttributeDrawerTypeMask() {
				return Serialization::SerializerTypeMask(
					Serialization::ItemSerializer::Type::STRING_VIEW_VALUE, 
					Serialization::ItemSerializer::Type::WSTRING_VIEW_VALUE);
			}
		}

		bool TextBoxAttributeDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			auto fail = [&](const auto&... message) {
				if (logger != nullptr)
					logger->Error("TextBoxAttributeDrawer::DrawObject - ", message...);
				return false;
			};

			if (object.Serializer() == nullptr)
				return fail("Serializer not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			Serialization::ItemSerializer::Type type = object.Serializer()->GetType();

			auto drawStringViewValue = [&](const std::string_view& currentText, const auto& setNewText) {
				static thread_local std::vector<char> textBuffer;
				{
					if (textBuffer.size() <= (currentText.length() + 1))
						textBuffer.resize(currentText.length() + 512);
					memcpy(textBuffer.data(), currentText.data(), currentText.length());
					textBuffer[currentText.length()] = '\0';
				}
				const std::string nameId = CustomSerializedObjectDrawer::DefaultGuiItemName(object, viewId);
				bool rv = ImGui::InputTextMultiline(nameId.c_str(), textBuffer.data(), textBuffer.size());
				DrawTooltip(nameId, object.Serializer()->TargetHint());
				if (strlen(textBuffer.data()) != currentText.length() || memcmp(textBuffer.data(), currentText.data(), currentText.length()) != 0)
					setNewText(std::string_view(textBuffer.data()));
				if (rv) ImGuiRenderer::FieldModified();
				return ImGui::IsItemDeactivatedAfterEdit();
			};

			if (type == Serialization::ItemSerializer::Type::STRING_VIEW_VALUE)
				return drawStringViewValue(object, [&](const std::string_view& newText) { object = newText; });
			else if (type == Serialization::ItemSerializer::Type::WSTRING_VIEW_VALUE) {
				const std::wstring_view wideView = object;
				const std::string wstringToString = Convert<std::string>(wideView);
				return drawStringViewValue(wstringToString, [&](const std::string_view& newText) {
					const std::wstring wideNewText = Convert<std::wstring>(newText);
					object = std::wstring_view(wideNewText);
					});
			}
			else return fail("Unsupported serializer type![File:", __FILE__, "; Line: ", __LINE__, "]");
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TextBoxAttributeDrawer>() {
		Editor::MainTextBoxAttributeDrawer()->Register(Editor::TextBoxAttributeDrawerTypeMask(), TypeId::Of<Serialization::TextBoxAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TextBoxAttributeDrawer>() {
		Editor::MainTextBoxAttributeDrawer()->Unregister(Editor::TextBoxAttributeDrawerTypeMask(), TypeId::Of<Serialization::TextBoxAttribute>());
	}
}
