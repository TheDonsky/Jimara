#include "DrawObjectPicker.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static std::string PointerKey(const std::string_view& name, Object* address) {
				std::stringstream stream;
				stream << name << "###DrawObjectPicker_select_object_" << ((size_t)address);
				return stream.str();
			}

			inline static std::string AssetName(const FileSystemDatabase::AssetInformation& info) {
				std::stringstream stream;
				stream << info.ResourceName()
					<< " [" << info.AssetRecord()->ResourceType().Name() << " From:'" << ((std::string)info.SourceFilePath()) << "']";
				return stream.str();
			}

			inline static std::string AssetName(Asset* asset, const TypeId& valueType, const FileSystemDatabase* assetDatabase) {
				FileSystemDatabase::AssetInformation assetInfo;
				if (assetDatabase != nullptr && assetDatabase->TryGetAssetInfo(asset, assetInfo))
					return AssetName(assetInfo);
				else {
					std::stringstream stream;
					stream << valueType.Name() << '<' << ((size_t)asset) << ">";
					return stream.str();
				}
			}

			inline static std::string ResourceName(Resource* resource, const TypeId& valueType, const FileSystemDatabase* assetDatabase) {
				Asset* asset = resource->GetAsset();
				if (asset == nullptr) {
					std::stringstream stream;
					stream << valueType.Name() << '<' << ((size_t)resource) << '>';
					return stream.str();
				}
				else return AssetName(asset, valueType, assetDatabase);
			}

			inline static std::string ComponentName(Component* component, Component* rootComponent) {
				const std::string& baseName = component->Name();
				if (component->Parent() == nullptr || component->Parent() == rootComponent)
					return baseName;
				std::string name = baseName + "]";
				while (true) {
					component = component->Parent();
					if ((component == nullptr) || (component->Parent() == component) || (component == rootComponent))
						break;
					name = component->Name() + "/" + name;
				}
				return baseName + " [" + name;
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
					Asset* asset = dynamic_cast<Asset*>(object);
					if (asset != nullptr) return AssetName(asset, valueType, assetDatabase);
				}
				{
					std::stringstream stream;
					if (object == nullptr)
						stream << "<None> (" << valueType.Name() << ")";
					else stream << valueType.Name() << '<' << ((size_t)object) << '>';
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

		bool DrawObjectPicker(
			const Serialization::SerializedObject& serializedObject, const std::string_view& serializedObjectId,
			OS::Logger* logger, Component* rootObject, const FileSystemDatabase* assetDatabase, std::vector<char>* searchBuffer) {
			// We need correct serializer type to be able to do anything with this:
			const Serialization::ObjectReferenceSerializer* const serializer = serializedObject.As<Serialization::ObjectReferenceSerializer>();
			if (serializer == nullptr) {
				if (logger != nullptr) logger->Error("DrawObjectPicker - Unsupported serializer type! <serializedObjectId:'", serializedObjectId, "'>");
				return false;
			}

			// Current value, type and stuff like that:
			const TypeId valueType = serializer->ReferencedValueType();
			const Reference<Object> currentObject = serializer->GetObjectValue(serializedObject.TargetAddr());
			const std::string currentObjectName = ObjectName(currentObject, valueType, rootObject, assetDatabase);
			bool pressed = ImGui::BeginCombo(serializedObjectId.data(), currentObjectName.data());
			DrawTooltip(serializedObjectId, serializer->TargetHint());
			if (!pressed)
				return false;
			Reference<Object> newSelection = currentObject;

			// Draw search bar:
			const std::string searchTerm = ToLower(InputSearchTerm(searchBuffer));

			// Draw null pointer as an option:
			{
				ImGui::Separator();
				bool wasSelected = (currentObject == nullptr);
				bool selected = wasSelected;
				static const std::string nameId = PointerKey("<None>", nullptr);
				ImGui::Selectable(nameId.c_str(), &selected);
				if ((!wasSelected) && selected) {
					newSelection = nullptr;
					ImGui::SetItemDefaultFocus();
				}
			}

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
				Asset* asset = (resource == nullptr) ? dynamic_cast<Asset*>(currentObject.operator->()) : resource->GetAsset();
				Reference<Asset> assetToLoad;
				assetDatabase->GetAssetsOfType<Resource>([&](const FileSystemDatabase::AssetInformation& info) {
					const bool isAsset = valueType.CheckType(info.AssetRecord());
					const bool isResource = info.AssetRecord()->ResourceType().IsDerivedFrom(valueType);
					if ((!isAsset) && (!isResource)) return;
					const std::string name = AssetName(info);
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
						if (isResource)
							assetToLoad = info.AssetRecord();
						else if (isAsset)
							newSelection = info.AssetRecord();
						ImGui::SetItemDefaultFocus();
					}
					}, false);
				if (assetToLoad != nullptr)
					newSelection = assetToLoad->LoadResource();
			}

			// Set new selection
			bool rv = (currentObject != newSelection);
			if (rv) {
				if (searchBuffer != nullptr && searchBuffer->size() > 0)
					searchBuffer->operator[](0) = '\0';
				serializer->SetObjectValue(newSelection, serializedObject.TargetAddr());
			}

			ImGui::EndCombo();
			if (rv) ImGuiRenderer::FieldModified();
			return rv;
		}
	}
}
