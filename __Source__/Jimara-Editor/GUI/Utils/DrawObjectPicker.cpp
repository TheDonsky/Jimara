#include "DrawObjectPicker.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static std::string PointerKey(const std::string_view& name, Object* address) {
				std::stringstream stream;
				stream << name << "###DrawObjectPicker_select_object_" << ((size_t)address);
				return stream.str();
			}

			inline static std::string ResourceName(const FileSystemDatabase::AssetInformation& info) {
				std::stringstream stream;
				stream << info.ResourceName()
					<< " [" << info.ResourceType().Name() << " From:'" << ((std::string)info.SourceFilePath()) << "']";
				return stream.str();
			}

			inline static std::string ResourceName(Resource* resource, const TypeId& valueType, const FileSystemDatabase* assetDatabase) {
				Asset* asset = resource->GetAsset();
				if (asset == nullptr) {
					std::stringstream stream;
					stream << valueType.Name() << '<' << ((size_t)resource) << '>';
					return stream.str();
				}
				FileSystemDatabase::AssetInformation assetInfo;
				if (assetDatabase != nullptr && assetDatabase->TryGetAssetInfo(asset, assetInfo))
					return ResourceName(assetInfo);
				else {
					std::stringstream stream;
					stream << valueType.Name() << '<' << ((size_t)resource) << "> from asset:<" << ((size_t)asset) << ">";
					return stream.str();
				}
			}

			inline static std::string ComponentName(Component* component, Component* rootComponent) {
				std::string name = component->Name();
				while (true) {
					component = component->Parent();
					if ((component == nullptr) || (component->Parent() == component) || (component == rootComponent))
						break;
					name = component->Name() + "/" + name;
				}
				return name;
			}

			inline static std::string ObjectName(Object* object, const TypeId& valueType, Component* rootComponent, const FileSystemDatabase* assetDatabase) {
				{
					Component* component = dynamic_cast<Component*>(object);
					if (component != nullptr) return ComponentName(component, rootComponent);
				}
				{
					Resource* resource = dynamic_cast<Resource*>(object);
					if (resource != nullptr) return ResourceName(resource, valueType, assetDatabase);
				}
				{
					std::stringstream stream;
					stream << valueType.Name() << '<' << ((size_t)object) << '>';
					return stream.str();
				}
			};

			inline static std::string ToLower(const std::string_view& view) {
				std::string result;
				result.resize(view.size());
				for (size_t i = 0; i < view.length(); i++)
					result[i] = std::tolower(view[i]);
				return result;
			}

			// Draws search bar
			inline static std::string_view InputSearchTerm(std::vector<char>* searchBuffer) {
				std::string_view searchTerm = "";
				if (searchBuffer != nullptr) {
					if (searchBuffer->size() <= 0)
						searchBuffer->resize(512, '\0');
					searchTerm = searchBuffer->data();
					if (searchBuffer->size() <= (searchTerm.length() + 2))
						searchBuffer->resize(searchTerm.length() + 512, '\0');
					ImGui::InputTextWithHint("Search", "Search by name", searchBuffer->data(), searchBuffer->size() - 1);
					searchTerm = searchBuffer->data();
				}
				return searchTerm;
			}
		}

		void DrawObjectPicker(
			const Serialization::SerializedObject& serializedObject, const std::string_view& serializedObjectId,
			OS::Logger* logger, Component* rootObject, const FileSystemDatabase* assetDatabase, std::vector<char>* searchBuffer) {
			// We need correct serializer type to be able to do anything with this:
			const Serialization::ObjectReferenceSerializer* const serializer = serializedObject.As<Serialization::ObjectReferenceSerializer>();
			if (serializer == nullptr) {
				if (logger != nullptr) logger->Error("DrawObjectPicker - Unsupported serializer type! <serializedObjectId:'", serializedObjectId, "'>");
				return;
			}

			// Current value, type and stuff like that:
			const TypeId valueType = serializer->ReferencedValueType();
			const Reference<Object> currentObject = serializer->GetObjectValue(serializedObject.TargetAddr());
			const std::string currentObjectName = ObjectName(currentObject, valueType, rootObject, assetDatabase);
			bool pressed = ImGui::BeginCombo(serializedObjectId.data(), currentObjectName.data());
			DrawTooltip(serializedObjectId, serializer->TargetHint());
			if (!pressed)
				return;
			Reference<Object> newSelection = currentObject;

			// Draw search bar:
			const std::string searchTerm = ToLower(InputSearchTerm(searchBuffer));

			// Draw objects:
			if (rootObject != nullptr) {
				bool firstObject = true;
				auto includeComponent = [&](Component* component) {
					if (!valueType.CheckType(component)) return;
					const std::string name = ComponentName(component, rootObject);
					if ((searchTerm.length() > 0) && (ToLower(name).find(searchTerm) == std::string::npos)) return;
					else if (firstObject) {
						ImGui::Separator();
						ImGui::Text("From Component Heirarchy:");
						firstObject = false;
					}
					const bool wasSelected = (static_cast<Object*>(component) == currentObject);
					bool selected = wasSelected;
					const std::string nameId = PointerKey(name, component);
					ImGui::Selectable(nameId.c_str(), &selected);
					if ((!wasSelected) && selected) {
						newSelection = component;
						ImGui::SetItemDefaultFocus();
					}
				};
				if (rootObject->RootObject() != rootObject)
					includeComponent(rootObject);
				{
					static thread_local std::vector<Component*> allChildComponents;
					allChildComponents.clear();
					rootObject->GetComponentsInChildren<Component>(allChildComponents, true);
					for (size_t i = 0; i < allChildComponents.size(); i++)
						includeComponent(allChildComponents[i]);
					allChildComponents.clear();
				}
			}

			// Draw resources:
			if (assetDatabase != nullptr) {
				bool firstObject = true;
				Resource* resource = dynamic_cast<Resource*>(currentObject.operator->());
				Asset* asset = (resource == nullptr) ? nullptr : resource->GetAsset();
				assetDatabase->GetAssetsOfType(valueType, [&](const FileSystemDatabase::AssetInformation& info) {
					const std::string name = ResourceName(info);
					if ((searchTerm.length() > 0) && (ToLower(name).find(searchTerm) == std::string::npos)) return;
					else if (firstObject) {
						ImGui::Separator();
						ImGui::Text("From Asset Database:");
						firstObject = false;
					}
					const bool wasSelected = (info.AssetRecord() == asset);
					bool selected = wasSelected;
					const std::string nameId = PointerKey(name, info.AssetRecord());
					ImGui::Selectable(nameId.c_str(), &selected);
					if ((!wasSelected) && selected) {
						newSelection = info.AssetRecord()->Load();
						ImGui::SetItemDefaultFocus();
					}
					}, false);
			}

			// Set new selection
			if (currentObject != newSelection) {
				if (searchBuffer != nullptr && searchBuffer->size() > 0)
					searchBuffer->operator[](0) = '\0';
				serializer->SetObjectValue(newSelection, serializedObject.TargetAddr());
			}

			ImGui::EndCombo();
		}
	}
}
