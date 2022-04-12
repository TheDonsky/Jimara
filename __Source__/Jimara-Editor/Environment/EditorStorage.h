#pragma once
#include "JimaraEditor.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// If you want for some objects to be persistent over multimple launches of the Editor,
		/// You can register it's type, add an instance of EditorStorageSerializer to it's type attributes,
		/// make sure it can be created with EditorContext as a constructor and it's kept as a storage object (EditorContext::AddStorageObject) throught it's lifetime.
		/// <para/> Note: EditorWindow and derived classes do EditorContext::AddStorageObject and EditorContext::RemoveStorageObject themselves, so do not call those manually... 
		///		(experience with other classes will likely differ as well)
		/// </summary>
		class EditorStorageSerializer : public virtual Serialization::SerializerList::From<Object> {
		public:
			/// <summary> Type, this serializer can serialize </summary>
			virtual TypeId StorageType()const = 0;

			/// <summary>
			/// Creates Editor Storage Object for EditorContext
			/// </summary>
			/// <param name="context"> Context of the Editor </param>
			/// <returns> Instance of a new Object </returns>
			virtual Reference<Object> CreateObject(EditorContext* context)const = 0;

			/// <summary>
			/// Simplifies code by implementing CreateObject and StorageType, as well as making GetFields type-specific
			/// </summary>
			/// <typeparam name="ObjectType"> Concrete type for StorageType/CreateObject </typeparam>
			template<typename ObjectType> class Of;

			/// <summary> Set of existing EditorStorageSerializers </summary>
			class Set;
		};

		/// <summary>
		/// Simplifies code by implementing CreateObject and StorageType, as well as making GetFields type-specific
		/// </summary>
		/// <typeparam name="ObjectType"> Concrete type for StorageType/CreateObject </typeparam>
		template<typename ObjectType>
		class EditorStorageSerializer::Of : public virtual EditorStorageSerializer {
		public:
			/// <summary> Type, this serializer can serialize </summary>
			virtual TypeId StorageType()const final override { return TypeId::Of<ObjectType>(); }

			/// <summary>
			/// Creates Editor Storage Object for EditorContext
			/// </summary>
			/// <param name="context"> Context of the Editor </param>
			/// <returns> Instance of a new Object </returns>
			inline virtual Reference<Object> CreateObject(EditorContext* context)const final override {
				return Object::Instantiate<ObjectType>(context);
			}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer and corresonding target as parameters </param>
			/// <param name="target"> Serializer target object </param>
			virtual std::enable_if_t<(!std::is_same_v<ObjectType, Object>), void>
				GetFields(const Callback<Serialization::SerializedObject>& recordElement, ObjectType* target)const = 0;

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer and corresonding target as parameters </param>
			/// <param name="target"> Serializer target object </param>
			virtual std::enable_if_t<(!std::is_same_v<ObjectType, Object>), void>
				GetFields(const Callback<Serialization::SerializedObject>& recordElement, Object* target)const final override {
				ObjectType* objectAddr = dynamic_cast<ObjectType*>(target);
				if (objectAddr == nullptr) return;
				return GetFields(recordElement, objectAddr);
			}
		};

		/// <summary> Set of existing EditorStorageSerializers </summary>
		class EditorStorageSerializer::Set : public virtual Object {
		public:
			/// <summary> Set of all currently existing EditorStorageSerializer objects </summary>
			static Reference<Set> All();

			/// <summary>
			/// Finds EditorStorageSerializer for given type
			/// </summary>
			/// <param name="typeName"> Type name </param>
			/// <returns> EditorStorageSerializer if found, nullptr otherwise </returns>
			const EditorStorageSerializer* FindSerializerOf(const std::string_view& typeName)const;

			/// <summary>
			/// Finds EditorStorageSerializer for given type
			/// </summary>
			/// <param name="typeIndex"> typeid(someObject) </param>
			/// <returns> EditorStorageSerializer if found, nullptr otherwise </returns>
			const EditorStorageSerializer* FindSerializerOf(const std::type_index& typeIndex)const;

			/// <summary>
			/// Finds EditorStorageSerializer for given type
			/// </summary>
			/// <param name="typeId"> Type identifier </param>
			/// <returns> EditorStorageSerializer if found, nullptr otherwise </returns>
			const EditorStorageSerializer* FindSerializerOf(const TypeId& typeId)const;

			/// <summary>
			/// Finds EditorStorageSerializer for given type
			/// </summary>
			/// <param name="item"> Object to serialize </param>
			/// <returns> EditorStorageSerializer if found, nullptr otherwise </returns>
			const EditorStorageSerializer* FindSerializerOf(const Object* item)const;

		private:
			// All serializers
			const std::vector<Reference<const EditorStorageSerializer>> m_serializers;

			// Mapping by type name
			const std::unordered_map<std::string_view, const EditorStorageSerializer*> m_typeNameToSerializer;

			// Mapping by type index
			const std::unordered_map<std::type_index, const EditorStorageSerializer*> m_typeIndexToSerializer;

			// Constructor
			Set(const std::map<std::string_view, Reference<const EditorStorageSerializer>>& typeIndexToSerializer);
		};
	}
}
