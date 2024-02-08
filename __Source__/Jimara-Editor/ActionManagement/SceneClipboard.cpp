#include "SceneClipboard.h"
#include <Jimara/Data/Serialization/Helpers/ComponentHeirarchySerializer.h>
#include <Jimara/Data/Serialization/Helpers/SerializeToJson.h>
#include <Jimara/OS/IO/Clipboard.h>


namespace Jimara {
	namespace Editor {
		struct SceneClipboard::Tools {
			inline static std::vector<Component*> FilterOverlappingHeirarchies(const std::vector<Reference<Component>>& roots) {
				// Turn roots into a set for fast lookup:
				std::unordered_set<Component*> rootArgs;
				for (size_t i = 0; i < roots.size(); i++) {
					Component* component = roots[i];
					if (component != nullptr)
						rootArgs.insert(component);
				}

				// Filter out objects that have parents in rootArgs:
				std::vector<Component*> rootLevelObjects;
				std::unordered_set<Component*> duplicates;
				for (size_t i = 0; i < roots.size(); i++) {
					Component* component = roots[i];

					if (component == nullptr) continue;
					else if (duplicates.find(component) != duplicates.end()) continue;

					Component* parent = component->Parent();
					while (parent != nullptr) {
						if (rootArgs.find(parent) != rootArgs.end()) break;
						else parent = parent->Parent();
					}

					if (parent == nullptr) {
						rootLevelObjects.push_back(component);
						duplicates.insert(component);
					}
				}
				return rootLevelObjects;
			}

			inline static const constexpr std::string_view ClipboardTypeId() { return "com.JimaraEditor.SceneClipboard_HeirarchyRecord"; }

			inline static void UnregisterComponent(SceneClipboard* self, Component* component) {
#ifndef NDEBUG
				if (component == nullptr) {
					self->m_context->Log()->Error("SceneClipboard::Tools::UnregisterComponent - Internal error: null component provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
#endif
				decltype(self->m_objectToId)::iterator idIt = self->m_objectToId.find(component);
				if (idIt == self->m_objectToId.end()) {
					self->m_context->Log()->Error("SceneClipboard::Tools::UnregisterComponent - Internal error: Component record not found! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				self->m_idToObject.erase(idIt->second);
				self->m_objectToId.erase(idIt);
				component->OnDestroyed() -= Callback(Tools::UnregisterComponent, self);
			}

			inline static void RegisterComponent(SceneClipboard* self, Component* component) {
#ifndef NDEBUG
				if (component == nullptr) {
					self->m_context->Log()->Error("SceneClipboard::Tools::RegisterComponent - Internal error: null component provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
#endif
				if (self->m_objectToId.find(component) != self->m_objectToId.end()) return;
				else if (component->Destroyed()) {
					self->m_context->Log()->Error(
						"SceneClipboard::Tools::RegisterComponent - Internal error('",
						component->Name(), "') Component already destroyed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return;
				}
				while (true) {
					const GUID guid = GUID::Generate();
					if (self->m_idToObject.find(guid) != self->m_idToObject.end()) continue; // Unnecessary safety, but anyway...
					self->m_objectToId[component] = guid;
					self->m_idToObject[guid] = component;
					break;
				}
				component->OnDestroyed() += Callback(Tools::UnregisterComponent, self);
			}

			inline static void RegisterAllComponents(SceneClipboard* self, Component* rootObject) {
				RegisterComponent(self, rootObject);
				for (size_t i = 0; i < rootObject->ChildCount(); i++)
					RegisterAllComponents(self, rootObject->GetChild(i));
			}

			inline static GUID GetExternalObjectId(SceneClipboard* self, Object* reference) {
				Component* const component = dynamic_cast<Component*>(reference);
				if (component == nullptr) return GUID::Null();
				const decltype(self->m_objectToId)::const_iterator it = self->m_objectToId.find(component);
				if (it == self->m_objectToId.end()) return GUID::Null();
				else return it->second;
			}

			inline static Reference<Object> GetExternalObject(SceneClipboard* self, const GUID& guid) {
				const decltype(self->m_idToObject)::const_iterator it = self->m_idToObject.find(guid);
				if (it == self->m_idToObject.end()) return nullptr;
				else return it->second;
			}
		};

		SceneClipboard::SceneClipboard(Scene::LogicContext* context) 
			: m_context(context) {
			assert(m_context != nullptr);
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			Tools::RegisterAllComponents(this, m_context->RootObject());
			m_context->OnComponentCreated() += Callback(Tools::RegisterComponent, this);
		}

		SceneClipboard::~SceneClipboard() {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
			m_context->OnComponentCreated() -= Callback(Tools::RegisterComponent, this);

			std::vector<Reference<Component>> all;
			for (decltype(m_objectToId)::const_iterator it = m_objectToId.begin(); it != m_objectToId.end(); ++it)
				all.push_back(it->first);
			
			for (size_t i = 0; i < all.size(); i++)
				Tools::UnregisterComponent(this, all[i]);
			
			if (!m_objectToId.empty())
				m_context->Log()->Error("SceneClipboard::~SceneClipboard - Internal error: m_objectToId not empty! ", __FILE__, "; Line: ", __LINE__, "]");
			if (!m_idToObject.empty())
				m_context->Log()->Error("SceneClipboard::~SceneClipboard - Internal error: m_idToObject not empty! ", __FILE__, "; Line: ", __LINE__, "]");
		}

		void SceneClipboard::CopyComponents(const std::vector<Reference<Component>>& roots) {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());

			// Filter out the heirarchies that may overlap:
			const std::vector<Component*> heirarchies = Tools::FilterOverlappingHeirarchies(roots);

			// Create an array of json-serialized heirarchies:
			nlohmann::json json = nlohmann::json::array();
			for (size_t i = 0; i < heirarchies.size(); i++) {
				ComponentHeirarchySerializerInput input = {};
				{
					input.rootComponent = heirarchies[i];
					input.context = m_context;
					input.assetDatabase = m_context->AssetDB();
					input.getExternalObjectId = Function(Tools::GetExternalObjectId, this);
					input.getExternalObject = Function(Tools::GetExternalObject, this);
				}
				bool error = false;
				nlohmann::json result = Serialization::SerializeToJson(
					ComponentHeirarchySerializer::Instance()->Serialize(input), m_context->Log(), error,
					[&](const Serialization::SerializedObject&, bool& error) -> nlohmann::json {
						m_context->Log()->Error("SceneClipboard::CopyComponents - ComponentHeirarchySerializer is not expected to have any Component references!");
						error = true;
						return {};
					});
				if (error)
					m_context->Log()->Error("SceneClipboard::CopyComponents - Failed to serialize heirarchy[", i, "]!");
				else json.push_back(result);
			}

			// Set text:
			const std::string jsonText = json.dump(1, '\t');
			OS::Clipboard::SetData(Tools::ClipboardTypeId(), MemoryBlock(jsonText.data(), jsonText.size(), nullptr), m_context->Log());
		}

		std::vector<Reference<Component>> SceneClipboard::PasteComponents(Component* parent) {
			std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());

			std::vector<Reference<Component>> results;
			if (parent == nullptr) return results;

			// Get text from the clipboard:
			const std::optional<std::string> jsonText = [&]() -> std::optional<std::string> {
				MemoryBlock block = OS::Clipboard::GetData(Tools::ClipboardTypeId(), m_context->Log());
				if (block.Data() == nullptr || block.Size() <= 0) return std::optional<std::string>();
				std::string rv;
				const char* ptr = reinterpret_cast<const char*>(block.Data());
				const char* end = ptr + block.Size();
				while (ptr < end) {
					const char symbol = (*ptr);
					if (symbol == 0) break;
					rv += symbol;
					ptr++;
				}
				return rv;
			}();
			if (!jsonText.has_value()) return results;

			// Extract json data:
			nlohmann::json json;
			try { json = nlohmann::json::parse(jsonText.value()); }
			catch (nlohmann::json::parse_error&) { return results; }
			if (!json.is_array()) return results;

			// Create components:
			for (size_t i = 0; i < json.size(); i++) {
				const nlohmann::json& heirarchy = json[i];
				ComponentHeirarchySerializerInput input = {};
				{
					input.rootComponent = Object::Instantiate<Component>(parent);
					input.context = m_context;
					input.assetDatabase = m_context->AssetDB();
					input.getExternalObjectId = Function(Tools::GetExternalObjectId, this);
					input.getExternalObject = Function(Tools::GetExternalObject, this);
				}
				if (!Serialization::DeserializeFromJson(
					ComponentHeirarchySerializer::Instance()->Serialize(input), heirarchy, m_context->Log(),
					[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
						m_context->Log()->Error("SceneClipboard::PasteComponents - ComponentHeirarchySerializer is not expected to have any Component references!");
						return false;
					})) {
					m_context->Log()->Error("SceneClipboard::PasteComponents - Failed to load scene snapshot!");
				}
				else if (input.rootComponent == nullptr)
					m_context->Log()->Error("SceneClipboard::PasteComponents - Component lost!");
				else results.push_back(input.rootComponent);
			}

			// We have the results:
			return results;
		}
	}
}
