#pragma once
#include "JimaraEditor.h"


namespace Jimara {
	namespace Editor {
		class EditorStorageSerializer : public virtual Serialization::SerializerList::From<Object> {
		public:
			virtual TypeId StorageType()const = 0;

			virtual Reference<Object> CreateObject(EditorContext* context)const = 0;

			template<typename ObjectType> class Of;

			class Set;
		};

		template<typename ObjectType>
		class EditorStorageSerializer::Of : public virtual EditorStorageSerializer {
		public:
			virtual TypeId StorageType()const final override { return TypeId::Of<ObjectType>(); }

			inline virtual Reference<Object> CreateObject(EditorContext* context)const final override {
				return Object::Instantiate<ObjectType>(context);
			}

			virtual std::enable_if_t<(!std::is_same_v<ObjectType, Object>), void>
				GetFields(const Callback<Serialization::SerializedObject>& recordElement, ObjectType* target)const = 0;

			virtual std::enable_if_t<(!std::is_same_v<ObjectType, Object>), void>
				GetFields(const Callback<Serialization::SerializedObject>& recordElement, Object* target)const final override {
				ObjectType* objectAddr = dynamic_cast<ObjectType*>(target);
				if (objectAddr == nullptr) return;
				return GetFields(recordElement, objectAddr);
			}
		};

		class EditorStorageSerializer::Set : public virtual Object {
		public:
			static Reference<Set> All();

			const EditorStorageSerializer* FindSerializerOf(const std::string_view& typeName)const;

			const EditorStorageSerializer* FindSerializerOf(const std::type_index& typeIndex)const;

			const EditorStorageSerializer* FindSerializerOf(const TypeId& typeId)const;

			const EditorStorageSerializer* FindSerializerOf(const Object* item)const;

		private:
			const std::vector<Reference<const EditorStorageSerializer>> m_serializers;

			const std::unordered_map<std::string_view, const EditorStorageSerializer*> m_typeNameToSerializer;

			const std::unordered_map<std::type_index, const EditorStorageSerializer*> m_typeIndexToSerializer;

			Set(const std::map<std::string_view, Reference<const EditorStorageSerializer>>& typeIndexToSerializer);
		};
	}
}
