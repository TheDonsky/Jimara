#include "EditorStorage.h"


namespace Jimara {
	namespace Editor {
		Reference<EditorStorageSerializer::Set> EditorStorageSerializer::Set::All() {
			static SpinLock allLock;
			static Reference<EditorStorageSerializer::Set> all;
			static Reference<RegisteredTypeSet> registeredTypes;

			std::unique_lock<SpinLock> lock(allLock);
			{
				const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
				if (currentTypes == registeredTypes) return all;
				else registeredTypes = currentTypes;
			}
			std::map<std::string_view, Reference<const EditorStorageSerializer>> typeIndexToSerializer;
			for (size_t i = 0; i < registeredTypes->Size(); i++) {
				void(*checkAttribute)(decltype(typeIndexToSerializer)*, const Object*) = [](decltype(typeIndexToSerializer)* serializers, const Object* attribute) {
					const EditorStorageSerializer* serializer = dynamic_cast<const EditorStorageSerializer*>(attribute);
					if (serializer != nullptr)
						(*serializers)[serializer->StorageType().Name()] = serializer;
				};
				registeredTypes->At(i).GetAttributes(Callback<const Object*>(checkAttribute, &typeIndexToSerializer));
			}
			all = new Set(typeIndexToSerializer);
			all->ReleaseRef();
			return all;
		}

		const EditorStorageSerializer* EditorStorageSerializer::Set::FindSerializerOf(const std::string_view& typeName)const {
			decltype(m_typeNameToSerializer)::const_iterator it = m_typeNameToSerializer.find(typeName);
			if (it == m_typeNameToSerializer.end()) return nullptr;
			else return it->second;
		}

		const EditorStorageSerializer* EditorStorageSerializer::Set::FindSerializerOf(const std::type_index& typeIndex)const {
			decltype(m_typeIndexToSerializer)::const_iterator it = m_typeIndexToSerializer.find(typeIndex);
			if (it == m_typeIndexToSerializer.end()) return nullptr;
			else return it->second;
		}

		const EditorStorageSerializer* EditorStorageSerializer::Set::FindSerializerOf(const TypeId& typeId)const {
			return FindSerializerOf(typeId.TypeIndex());
		}

		const EditorStorageSerializer* EditorStorageSerializer::Set::FindSerializerOf(const Object* item)const {
			if (item == nullptr) return nullptr;
			else return FindSerializerOf(typeid(*item));
		}

		EditorStorageSerializer::Set::Set(const std::map<std::string_view, Reference<const EditorStorageSerializer>>& typeIndexToSerializer)
			: m_serializers([&]()-> std::vector<Reference<const EditorStorageSerializer>> {
			std::vector<Reference<const EditorStorageSerializer>> list;
			for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
				list.push_back(it->second);
			return list;
				}()), m_typeNameToSerializer([&]() -> std::unordered_map<std::string_view, const EditorStorageSerializer*> {
					std::unordered_map<std::string_view, const EditorStorageSerializer*> map;
					for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
						map[it->second->StorageType().Name()] = it->second;
					return map;
					}()), m_typeIndexToSerializer([&]() -> std::unordered_map<std::type_index, const EditorStorageSerializer*> {
						std::unordered_map<std::type_index, const EditorStorageSerializer*> map;
						for (auto it = typeIndexToSerializer.begin(); it != typeIndexToSerializer.end(); ++it)
							map[it->second->StorageType().TypeIndex()] = it->second;
						return map;
						}()) {
			assert(m_serializers.size() == typeIndexToSerializer.size());
			assert(m_typeNameToSerializer.size() == typeIndexToSerializer.size());
			assert(m_typeIndexToSerializer.size() == typeIndexToSerializer.size());
		}
	}
}
