#pragma once
#include "../../Math/Math.h"
#include "../../Core/TypeRegistration/TypeRegistartion.h"
#include <string_view>
#include <cassert>

#pragma warning(disable: 4250)
namespace Jimara {
	namespace Serialization {
		/**
		* Hereby leith the basic set of primitive blocks the Scene serializers/deserializers and the editor (inspector) view
		* will use for scene store/load and exposed parameter manipulation.
		* 
		* All serializers comply to this basic idea: 
		* The serializer objects are shared between objects/resources that we're targeting, but are, generally speaking, 
		* tied to their target class and serve to expose their user-modifiable internals and/or externals through the interfaces known by the engine.
		* 
		* For example, let's say, we have the following class we wish to serialize:
		* struct SomeStruct {
		*	// We need to store these:
		*	int intVar;
		*	float floatVar;
		*	Vector3 vecVar;
		* 
		*	// This is used during runtime and there's no need to store this one
		*	int hiddenVar;
		* };
		* 
		* Serializer for the structure could look something like this:
		* class SomeStructSerializer : public virtual SerializerList::From<SomeStruct> {
		* public:
		*	// This time, we only need one instance of the serializer, so the private constructor will prevent unnecessary instantiation
		*	inline SomeStructSerializer(
		*		const std::string_view& name = "Name of the variable (value ignored by everything, visible through the editor and may appear as a hint for some serialized formats)",
		*		const std::string_view& hint = "Some snarky comment about 'SomeStruct' or an adequate decription of it, if you prefer it that way (Relevant to Editor only)"),
		*		const std::vector<Reference<const Object>>& attributes = {} // Some attributes may go here
		*		: ItemSerializer(name, hint) {}
		* 
		*	// We'll use SerializerList interface to expose relevant data
		*	inline virtual void GetFields(const Callback<SerializedObject>& recordElement, SomeStruct* target)const override {
		*		static const Reference<const ItemSerializer::Of<int>> intVarSerializer = IntSerializer::Create(
		*			"intVar", "This comment should appear on the inspector, when the cursor hovers above the variable");
		*		recordElement(intVarSerializer->Serialize(&target->intVar));
		* 
		*		// We can add some additional attributes in case we wanted to, let's say, expose this float as a slider or something like that
		*		static const FloatSliderAttribute slider(0.0f, 1.0f);
		*		static const std::vector<Reference<const Object>> floatVarAttributes = { &slider };
		*		static const Reference<const ItemSerializer::Of<float>> floatVarSerializer = FloatSerializer::Create(
		*			"floatVar", "Yep, mouse hover should reveal this too", floatVarAttributes);
		*		recordElement(floatVarSerializer->Serialize(target->floatVar));
		* 
		*		// 'Hints' are optional
		*		static const Reference<const ItemSerializer::Of<Vector3>> vecVarSerializer = Vector3Serializer::Create("vecVar");
		*		recordElement(vecVarSerializer->Serialize(&target->vecVar));
		*	}
		* };
		* 
		* And finally, the simplified version of internal usage by the engine would look something like this:
		* {
		*	SomeStruct* target;
		*	SomeStructSerializer* serializer;
		*	serializer->GetFields(perFieldLogicCallback, (void*)target);
		* }
		* 
		* Of cource, generally speaking, the engine will be unaware of the specifics of the exact SomeStruct SomeStructSerializer definitions, 
		* it'll find it though some registries and alike, but idea is the same regardless...
		* 
		* Note that if your custom serializers are neither simple combinations of the ones provided by the engine or implementations of their virtual interfaces,
		* the engine & editor infrustructure will more than likely fail to utilize them correctly, unless you edit their source or provide additional hooks.
		*/



		class SerializerList;
		class ObjectReferenceSerializer;
		template<typename ValueType> class ValueSerializer;

		/// <summary>
		/// Parent class of all item/object/resource serializers.
		/// </summary>
		class JIMARA_API ItemSerializer : public virtual Object {
		public:
			/// <summary>
			/// Serializer type identifiers known to the engine
			/// </summary>
			enum class Type : uint8_t {
				/// <summary> ValueSerializer<bool> will return this </summary>
				BOOL_VALUE = 0,

				/// <summary> ValueSerializer<char> will return this </summary>
				CHAR_VALUE = 1,

				/// <summary> ValueSerializer<signed char> will return this </summary>
				SCHAR_VALUE = 2,

				/// <summary> ValueSerializer<unsigned char> will return this </summary>
				UCHAR_VALUE = 3,

				/// <summary> ValueSerializer<wchar_t> will return this </summary>
				WCHAR_VALUE = 4,

				/// <summary> ValueSerializer<short> will return this </summary>
				SHORT_VALUE = 5,

				/// <summary> ValueSerializer<unsigned short> will return this </summary>
				USHORT_VALUE = 6,

				/// <summary> ValueSerializer<int> will return this </summary>
				INT_VALUE = 7,

				/// <summary> ValueSerializer<unsigned int> will return this </summary>
				UINT_VALUE = 8,

				/// <summary> ValueSerializer<long> will return this </summary>
				LONG_VALUE = 9,

				/// <summary> ValueSerializer<unsigned long> will return this </summary>
				ULONG_VALUE = 10,

				/// <summary> ValueSerializer<long long> will return this </summary>
				LONG_LONG_VALUE = 11,

				/// <summary> ValueSerializer<unsigned long long> will return this </summary>
				ULONG_LONG_VALUE = 12,

				/// <summary> ValueSerializer<float> will return this </summary>
				FLOAT_VALUE = 13,

				/// <summary> ValueSerializer<double> will return this </summary>
				DOUBLE_VALUE = 14,

				/// <summary> ValueSerializer<Vector2> will return this </summary>
				VECTOR2_VALUE = 15,

				/// <summary> ValueSerializer<Vector3> will return this </summary>
				VECTOR3_VALUE = 16,

				/// <summary> ValueSerializer<Vector4> will return this </summary>
				VECTOR4_VALUE = 17,

				/// <summary> ValueSerializer<Matrix2> will return this </summary>
				MATRIX2_VALUE = 18,

				/// <summary> ValueSerializer<Matrix3> will return this </summary>
				MATRIX3_VALUE = 19,

				/// <summary> ValueSerializer<Matrix4> will return this </summary>
				MATRIX4_VALUE = 20,

				/// <summary> ValueSerializer<std::string_view> will return this </summary>
				STRING_VIEW_VALUE = 21,

				/// <summary> ValueSerializer<str::wstring_view> will return this </summary>
				WSTRING_VIEW_VALUE = 22,

				/// <summary> ValueSerializer<pointer to an object type> will return this </summary>
				OBJECT_PTR_VALUE = 23,

				/// <summary> SerializerList will return this </summary>
				SERIALIZER_LIST = 24,

				/// <summary> Not a valid option; just a number of valid values </summary>
				SERIALIZER_TYPE_COUNT = 25,

				/// <summary> Invalid type (no ItemSerializer should be of this type) </summary>
				ERROR_TYPE = SERIALIZER_TYPE_COUNT
			};

			/// <summary>
			/// Tranlates ItemSerializer::Type to corresponding ItemSerializer TypeId
			/// </summary>
			/// <param name="type"> ItemSerializer::Type </param>
			/// <returns> TypeId </returns>
			inline static TypeId TypeToTypeId(Type type);

		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			inline ItemSerializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) 
				: m_name(name), m_hint(hint), m_attributes(attributes) {}

			/// <summary> This should return what type of a serializer we're dealing with (Engine internals will only acknowledge SerializerList and ValueSerializer<>) </summary>
			inline Type GetType()const {
				Type type = GetSerializerType();
#ifndef NDEBUG
				// Protection against 'Fake values'
				TypeId id = TypeToTypeId(type);
				assert(id != TypeId::Of<void>() && id.CheckType(this));
#endif
				return type;
			}

			/// <summary> Target type name </summary>
			inline const std::string& TargetName()const { return m_name; }

			/// <summary> Target hint (editor helper texts on hover and what not) </summary>
			inline const std::string& TargetHint()const { return m_hint; }

			/// <summary> Number of serializer attributes </summary>
			inline size_t AttributeCount()const { return m_attributes.size(); }

			/// <summary> 
			/// Serializer attribute by index
			/// </summary>
			/// <param name="index"> Attribute index </param>
			/// <returns> Attribute </returns>
			inline const Object* Attribute(size_t index)const { return m_attributes[index]; }

			/// <summary>
			/// Searches for an attribute by type
			/// </summary>
			/// <typeparam name="AttributeType"> Attribute type </typeparam>
			/// <param name="index"> Optionally, the attribute index can be written to this address </param>
			/// <returns> Attribute of the given type if found, nullptr otherwise </returns>
			template<typename AttributeType>
			inline const AttributeType* FindAttributeOfType(size_t* index = nullptr)const {
				for (size_t i = 0; i < m_attributes.size(); i++) {
					const AttributeType* attr = dynamic_cast<const AttributeType*>(Attribute(i));
					if (attr != nullptr) {
						if (index != nullptr) (*index) = i;
						return attr;
					}
				}
				if (index != nullptr) (*index) = m_attributes.size();
				return nullptr;
			}

			// ItemSerializer that knows how to interpret user data
			template<typename TargetAddrType> class Of;

			// Parent class for ValueSerializer<ValueType> to add interface
			template<typename ValueType> class BaseValueSerializer;
			template<typename ValueType> class BaseValueSerializer<ValueType*>;

		protected:
			/// <summary> This should return what type of a serializer we're dealing with (Engine internals will only acknowledge SerializerList and ValueSerializer<>) </summary>
			virtual Type GetSerializerType()const = 0;

		private:
			// Target type name
			const std::string m_name;

			// Target hint (editor helper texts on hover and what not)
			const std::string m_hint;

			// Serializer attributes
			const std::vector<Reference<const Object>> m_attributes;
		};


		/// <summary>
		/// Serializer for Object references
		/// </summary>
		class ObjectReferenceSerializer : public virtual ItemSerializer {
		private:
			// Only ValueSerializer and BaseValueSerializer can access the constructor
			inline ObjectReferenceSerializer() {}
			template<typename ValueType> friend class ValueSerializer;
			template<typename ValueType> friend class ItemSerializer::BaseValueSerializer;

		public:
			/// <summary>
			/// Type of the Object, GetObjectValue() can return and SetObjectValue() can set sucessufully
			/// </summary>
			virtual TypeId ReferencedValueType()const = 0;

			/// <summary>
			/// Gets the pointer value casted to Object*
			/// </summary>
			/// <param name="targetAddr"> Target object address </param>
			/// <returns> Object </returns>
			virtual Object* GetObjectValue(void* targetAddr)const = 0;

			/// <summary>
			/// Sets the pointer value to the object
			/// </summary>
			/// <param name="object"> Value to set </param>
			/// <param name="targetAddr"> Target object address </param>
			virtual void SetObjectValue(Object* object, void* targetAddr)const = 0;
		};

		/// <summary>
		/// Base interface for ValueSerializer<ValueType>
		/// </summary>
		/// <typeparam name="ValueType"> Type, ValueSerializer evaluates as </typeparam>
		template<typename ValueType> 
		class ItemSerializer::BaseValueSerializer : public virtual ItemSerializer {
		private:
			// Only ValueSerializer can access the constructor
			inline BaseValueSerializer() {}
			friend class ValueSerializer<ValueType>;

		public:
			/// <summary>
			/// Gets value from target
			/// </summary>
			/// <param name="targetAddr"> Serializer target object address </param>
			/// <returns> Stored value </returns>
			virtual ValueType Get(void* targetAddr)const = 0;

			/// <summary>
			/// Sets target value
			/// </summary>
			/// <param name="value"> Value to set </param>
			/// <param name="targetAddr"> Serializer target object address </param>
			virtual void Set(ValueType value, void* targetAddr)const = 0;

			/// <summary> Serializer type for this ValueType </summary>
			inline static constexpr Type SerializerType() {
				constexpr const Type type =
					std::is_same_v<ValueType, bool> ? Type::BOOL_VALUE :
					std::is_same_v<ValueType, char> ? Type::CHAR_VALUE :
					std::is_same_v<ValueType, signed char> ? Type::SCHAR_VALUE :
					std::is_same_v<ValueType, unsigned char> ? Type::UCHAR_VALUE :
					std::is_same_v<ValueType, wchar_t> ? Type::WCHAR_VALUE :
					std::is_same_v<ValueType, short> ? Type::SHORT_VALUE :
					std::is_same_v<ValueType, unsigned short> ? Type::USHORT_VALUE :
					std::is_same_v<ValueType, int> ? Type::INT_VALUE :
					std::is_same_v<ValueType, unsigned int> ? Type::UINT_VALUE :
					std::is_same_v<ValueType, long> ? Type::LONG_VALUE :
					std::is_same_v<ValueType, unsigned long> ? Type::ULONG_VALUE :
					std::is_same_v<ValueType, long long> ? Type::LONG_LONG_VALUE :
					std::is_same_v<ValueType, unsigned long long> ? Type::ULONG_LONG_VALUE :
					std::is_same_v<ValueType, float> ? Type::FLOAT_VALUE :
					std::is_same_v<ValueType, double> ? Type::DOUBLE_VALUE :
					std::is_same_v<ValueType, Vector2> ? Type::VECTOR2_VALUE :
					std::is_same_v<ValueType, Vector3> ? Type::VECTOR3_VALUE :
					std::is_same_v<ValueType, Vector4> ? Type::VECTOR4_VALUE :
					std::is_same_v<ValueType, Matrix2> ? Type::MATRIX2_VALUE :
					std::is_same_v<ValueType, Matrix3> ? Type::MATRIX3_VALUE :
					std::is_same_v<ValueType, Matrix4> ? Type::MATRIX4_VALUE :
					std::is_same_v<ValueType, std::string_view> ? Type::STRING_VIEW_VALUE :
					std::is_same_v<ValueType, std::wstring_view> ? Type::WSTRING_VIEW_VALUE :
					Type::ERROR_TYPE;
				static_assert(type != Type::ERROR_TYPE);
				return type;
			}
		};
		
		/// <summary>
		/// Base interface for ValueSerializer<ValueType*>
		/// </summary>
		/// <typeparam name="ValueType"> Type, ValueSerializer evaluates as a pointer of </typeparam>
		template<typename ValueType> 
		class ItemSerializer::BaseValueSerializer<ValueType*> : public virtual ObjectReferenceSerializer {
		private:
			// Only ValueSerializer can access the constructor
			inline BaseValueSerializer() {}
			friend class ValueSerializer<ValueType*>;

		public:
			/// <summary> Serializer type for this ValueType </summary>
			inline static constexpr Type SerializerType() {
				constexpr const Type type = std::is_base_of_v<Object, ValueType> ? Type::OBJECT_PTR_VALUE : Type::ERROR_TYPE;
				static_assert(type != Type::ERROR_TYPE);
				return type;
			}

			/// <summary>
			/// Gets value from target
			/// </summary>
			/// <param name="targetAddr"> Serializer target object address </param>
			/// <returns> Stored value </returns>
			virtual ValueType* Get(void* targetAddr)const = 0;

			/// <summary>
			/// Sets target value
			/// </summary>
			/// <param name="value"> Value to set </param>
			/// <param name="targetAddr"> Serializer target object address </param>
			virtual void Set(ValueType* value, void* targetAddr)const = 0;

			/// <summary>
			/// Type of the Object, GetObjectValue() can return and SetObjectValue() can set sucessufully
			/// </summary>
			inline virtual TypeId ReferencedValueType()const final override { return TypeId::Of<ValueType>(); }

			/// <summary>
			/// Gets the pointer value casted to Object*
			/// </summary>
			/// <param name="targetAddr"> Target object address </param>
			/// <returns> Object </returns>
			inline virtual Object* GetObjectValue(void* targetAddr)const final override {
				return dynamic_cast<Object*>(Get(targetAddr));
			}

			/// <summary>
			/// Sets the pointer value to the object
			/// </summary>
			/// <param name="object"> Value to set </param>
			/// <param name="targetAddr"> Target object address </param>
			inline virtual void SetObjectValue(Object* object, void* targetAddr)const final override {
				Set(dynamic_cast<ValueType*>(object), targetAddr);
			}
		};


		/// <summary>
		/// Pair of an ItemSerializer and corresponding target address
		/// </summary>
		class JIMARA_API SerializedObject {
		private:
			// Serializer for target
			const ItemSerializer* m_serializer;

			// Serializer target
			void* m_targetAddr;

			// ItemSerializer::Of can access the constructor
			template<typename TargetAddrType>
			friend class ItemSerializer::Of;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <typeparam name="TargetAddrType"> Type of the targetAddr </typeparam>
			/// <param name="ser"> Serializer for target </param>
			/// <param name="target"> Serializer target </param>
			template<typename TargetAddrType>
			inline SerializedObject(const ItemSerializer* serializer, TargetAddrType* target)
				: m_serializer(serializer), m_targetAddr((void*)target) {
#ifndef NDEBUG
				if (m_serializer != nullptr)
					assert(m_serializer->GetType() != ItemSerializer::Type::ERROR_TYPE);
#endif
			}

		public:
			/// <summary> Default constructor </summary>
			inline SerializedObject() : m_serializer(nullptr), m_targetAddr(nullptr) {}

			/// <summary> Serializer for target </summary>
			inline const ItemSerializer* Serializer()const { return m_serializer; };

			/// <summary> Serializer target </summary>
			void* TargetAddr()const { return m_targetAddr; };

			/// <summary>
			/// Type casts serializer to given type
			/// </summary>
			/// <typeparam name="SerializerType"> Type to cast serializer to </typeparam>
			/// <returns> SerializerType address if serializer is of a correct type </returns>
			template<typename SerializerType>
			inline const SerializerType* As()const { return dynamic_cast<const SerializerType*>(m_serializer); }

			/// <summary>
			/// Type-casts the serializer to ValueSerializer<ValueType> and retrieves value
			/// Note: Will crash if the serializer is not of a correct type
			/// </summary>
			/// <typeparam name="ValueType"> ValueSerializer's ValueType </typeparam>
			template<typename ValueType>
			inline operator ValueType()const;

			/// <summary>
			/// Type-casts the serializer to ValueSerializer<ValueType> and sets the value
			/// Note: Will crash if the serializer is not of a correct type
			/// </summary>
			/// <typeparam name="ValueType"> ValueSerializer's ValueType </typeparam>
			/// <param name="value"> Value to set </param>
			template<typename ValueType>
			inline void operator=(const ValueType& value)const;

			/// <summary>
			/// Type-casts the serializer to SerializerList and invokes GetFields() with the callback and the TargetAddr
			/// <para/> Note: Will crash if the serializer is not of a correct type
			/// </summary>
			/// <typeparam name="RecordCallback"> Anything, that can be called with a Serialization::SerializedObject as an argument  </typeparam>
			/// <param name="callback"> Callback for SerializerList::GetFields() </param>
			template<typename RecordCallback>
			inline void GetFields(const RecordCallback& callback)const;

			/// <summary>
			/// Type-casts to ObjectReferenceSerializer and retrieves the object
			/// <para/> Note: Will crash if the serializer is not of a correct type
			/// </summary>
			/// <returns> Object </returns>
			inline Object* GetObjectValue()const {
				const ObjectReferenceSerializer* serializer = As<ObjectReferenceSerializer>();
#ifndef NDEBUG
				// Make sure the user has some idea why we crashed:
				assert(serializer != nullptr);
#endif
				return serializer->GetObjectValue(TargetAddr());
			}

			/// <summary>
			/// Type-casts to ObjectReferenceSerializer and sets the object
			/// <para/> Note: Will crash if the serializer is not of a correct type
			/// </summary>
			/// <param name="object"> Value to set </param>
			inline void SetObjectValue(Object* object)const {
				const ObjectReferenceSerializer* serializer = As<ObjectReferenceSerializer>();
#ifndef NDEBUG
				// Make sure the user has some idea why we crashed:
				assert(serializer != nullptr);
#endif
				serializer->SetObjectValue(object, TargetAddr());
			}
		};

		/// <summary>
		/// ItemSerializer that knows how to interpret user data
		/// </summary>
		/// <typeparam name="TargetAddrType"></typeparam>
		template<typename TargetAddrType>
		class ItemSerializer::Of : public virtual ItemSerializer {
		public:
			/// <summary> Type definition for target type </summary>
			typedef TargetAddrType TargetType;

			/// <summary> This class is virtual, so no need for a constructor as such </summary>
			inline virtual ~Of() = 0;

			/// <summary>
			/// Creates a SerializedObject safetly
			/// </summary>
			/// <param name="target"> Target object </param>
			/// <returns> SerializedObject </returns>
			inline SerializedObject Serialize(TargetAddrType* target)const { return SerializedObject(this, target); }

			/// <summary>
			/// Creates a SerializedObject safetly
			/// </summary>
			/// <param name="target"> Target object </param>
			/// <returns> SerializedObject </returns>
			inline SerializedObject Serialize(TargetAddrType& target)const { return SerializedObject(this, &target); }
		};

		/// <summary> This class is virtual, so no need for a constructor as such </summary>
		template<typename TargetAddrType>
		inline ItemSerializer::Of<TargetAddrType>::~Of() {}



		/// <summary>
		/// Interface for providing a list of sub-objects and properties for serialization
		/// <para/> Note:
		///		<para/> This will likely be your primary way of dealling with custom class serialization.
		///		<para/> In order to utilize this interface properly, you should pay close attention to the way engine treats the fields:
		///		<para/> 0. Any field, can be a sub-serializer of SerializerList or ValueSerializer<scalar/vector> types;
		///		<para/> 1. Field names and hints are just used for displaying the values in editor and are included in text-serialized files for readability; 
		///			they hold no other significance when serializing/deserializing;
		///		<para/> 2. Only thing that actually matters when extracting data from serialized files, is the order of the fields and their types; 
		///			the names are mostly ignored to maintain performance;
		///		<para/> 3. If the custom structure has a fixed set of fields, 'hard-coding' the order is easy enough, but if the number of fields vary, 
		///			making sure the previously reported fields define what comes next will be crucial to maintain the internal consistency of the data structure;
		///		<para/> 4. Once again, sub-serializer names DO NOT MATTER, they can have duplicates, they're free to change from call to call, and this class is to be treated 
		///			like a list with fixed order, not a map of any kind...
		///		<para/> 5. There are a few exceptions when the names are relevant, for example, the Animations are tied to fields based on names, but that is not the expected norm.
		/// </summary>
		class JIMARA_API SerializerList : public virtual ItemSerializer {
		public:
			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object address </param>
			virtual void GetFields(const Callback<SerializedObject>& recordElement, void* targetAddr)const = 0;

			/// <summary>
			/// SerializerList that gets a concrete type as targetAddr
			/// </summary>
			/// <typeparam name="TargetAddrType"> Type of the targetAddr </typeparam>
			template<typename TargetAddrType>
			class From;

		protected:
			/// <summary> Type::SERIALIZER_LIST </summary>
			virtual Type GetSerializerType()const override { return Type::SERIALIZER_LIST; }

		private:
			// Only 'From<>' can inherit from SerializerList, so the constructor is private
			inline SerializerList() {}
		};

		/// <summary>
		/// Interface for providing a list of sub-objects and properties for serialization
		/// </summary>
		/// <typeparam name="TargetAddrType"> Type of the targetAddr </typeparam>
		template<typename TargetAddrType>
		class SerializerList::From : public SerializerList, public virtual ItemSerializer::Of<TargetAddrType> {
		protected:
			/// <summary> Type::SERIALIZER_LIST </summary>
			virtual Type GetSerializerType()const final override { return SerializerList::GetSerializerType(); }

		public:
			/// <summary> Constructor </summary>
			inline From() : SerializerList() {}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object </param>
			virtual void GetFields(const Callback<SerializedObject>& recordElement, TargetAddrType* target)const = 0;

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object address </param>
			inline virtual void GetFields(const Callback<SerializedObject>& recordElement, void* targetAddr)const final override {
				GetFields(recordElement, (TargetAddrType*)targetAddr);
			}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <typeparam name="RecordCallback"> Anything, that can be called with a Serialization::SerializedObject as an argument </typeparam>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="target"> Object to serialize </param>
			template<typename RecordCallback>
			inline void GetFields(const RecordCallback& recordElement, TargetAddrType* target)const {
				void(*serialize)(const RecordCallback*, Serialization::SerializedObject) = [](const RecordCallback* record, Serialization::SerializedObject field) { (*record)(field); };
				GetFields(Callback<Serialization::SerializedObject>(serialize, &recordElement), target);
			}
		};



		/// <summary>
		/// Concrete serializer for storing scalar and vector types
		/// Notes: Even if this is a template, only the set amount of types are known by the engine and all of those have separate type definitions down below.
		/// </summary>
		/// <typeparam name="ValueType"> Scalar/Vector value, as well as some other simple/built-in classes like string </typeparam>
		template<typename ValueType>
		class ValueSerializer : public virtual ItemSerializer, public virtual ItemSerializer::BaseValueSerializer<ValueType> {
		public:
			/// <summary>
			/// ValueSerializer that knows how to interpret user data
			/// </summary>
			/// <typeparam name="TargetAddrType"> Type of the targetAddr </typeparam>
			/// <typeparam name="ValueType"> Scalar/Vector value, as well as some other simple/built-in classes like string </typeparam>
			template<typename TargetAddrType>
			class From;

			/// <summary>
			/// Creates an instance of a ValueSerializer
			/// </summary>
			/// <typeparam name="TargetAddrType"> Type of the user data </typeparam>
			/// <param name="name"> Field name </param>
			/// <param name="hint"> Field hint/short description </param>
			/// <param name="getValue"> Value get function </param>
			/// <param name="setValue"> Value set function </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> New instance of a ValueSerializer </returns>
			template<typename TargetAddrType>
			inline static Reference<const From<TargetAddrType>> Create(
				const std::string_view& name, const std::string_view& hint,
				const Function<ValueType, TargetAddrType*>& getValue, const Callback<const ValueType&, TargetAddrType*>& setValue,
				const std::vector<Reference<const Object>>& attributes = {}) {
				Reference<const ValueSerializer> instance = new From<TargetAddrType>(name, hint, getValue, setValue, attributes);
				instance->ReleaseRef();
				return instance;
			}

			/// <summary>
			/// Creates an instance of a ValueSerializer
			/// </summary>
			/// <typeparam name="TargetAddrType"> Type of the user data </typeparam>
			/// <typeparam name="GetterType"> Arbitrary type, Function<ValueType, TargetAddrType*> can be constructed from </typeparam>
			/// <typeparam name="SetterType"> Arbitrary type, Callback<const ValueType&, TargetAddrType*> can be constructed from </typeparam>
			/// <param name="name"> Field name </param>
			/// <param name="hint"> Field hint/short description </param>
			/// <param name="getValue"> Value get function </param>
			/// <param name="setValue"> Value set function </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> New instance of a ValueSerializer </returns>
			template<typename TargetAddrType, typename GetterType, typename SetterType>
			inline static Reference<const From<TargetAddrType>> For(
				const std::string_view& name, const std::string_view& hint,
				const GetterType& getValue, const SetterType& setValue,
				const std::vector<Reference<const Object>>& attributes = {}) {
				return Create<TargetAddrType>(
					name, hint, 
					Function<ValueType, TargetAddrType*>(getValue),
					Callback<const ValueType&, TargetAddrType*>(setValue),
					attributes);
			}

			/// <summary>
			/// Creates an instance of a ValueSerializer
			/// </summary>
			/// <param name="name"> Field name </param>
			/// <param name="hint"> Field hint/short description </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> New instance of a ValueSerializer </returns>
			inline static Reference<const From<ValueType>> Create(
				const std::string_view& name, const std::string_view& hint = "",
				const std::vector<Reference<const Object>>& attributes = {}) {
				return For<ValueType>(
					name, hint,
					[](ValueType* targetAddr) -> ValueType { return *targetAddr; },
					[](const ValueType& value, ValueType* targetAddr) { (*targetAddr) = value; },
					attributes);
			}

		protected:
			/// <summary> Serializer type </summary>
			virtual Type GetSerializerType()const final override { return ItemSerializer::BaseValueSerializer<ValueType>::SerializerType(); }

		private:
			// Constructor is private to prevent a chance of someone creating derived classes and making logic a lot more convoluted than it has to be
			inline ValueSerializer() {}
		};

		/// <summary>
		/// ValueSerializer that knows how to interpret user data
		/// </summary>
		/// <typeparam name="TargetAddrType"> Type of the targetAddr </typeparam>
		/// <typeparam name="ValueType"> Scalar/Vector value, as well as some other simple/built-in classes like string </typeparam>
		template<typename ValueType>
		template<typename TargetAddrType>
		class ValueSerializer<ValueType>::From final : public virtual ValueSerializer<ValueType>, public virtual ItemSerializer::Of<TargetAddrType> {
		public:
			/// <summary>
			/// Gets value from target
			/// </summary>
			/// <param name="targetAddr"> Serializer target object address </param>
			/// <returns> Stored value </returns>
			inline virtual ValueType Get(void* targetAddr)const final override { return m_getValue((TargetAddrType*)targetAddr); }

			/// <summary>
			/// Sets target value
			/// </summary>
			/// <param name="value"> Value to set </param>
			/// <param name="targetAddr"> Serializer target object address </param>
			inline virtual void Set(ValueType value, void* targetAddr)const final override { m_setValue(value, (TargetAddrType*)targetAddr); }

		private:
			// Getter
			const Function<ValueType, TargetAddrType*> m_getValue;

			// Setter
			const Callback<const ValueType&, TargetAddrType*> m_setValue;

			// Constructor is private to prevent a chance of someone creating derived classes and making logic a lot more convoluted than it has to be
			inline From(
				const std::string_view& name, const std::string_view& hint,
				const Function<ValueType, TargetAddrType*>& getValue, const Callback<const ValueType&, TargetAddrType*>& setValue,
				const std::vector<Reference<const Object>>& attributes = {})
				: ItemSerializer(name, hint, attributes), m_getValue(getValue), m_setValue(setValue) {}

			// ValueSerializer is allowed to create instances:
			friend class ValueSerializer;
		};

		/// <summary>
		/// Tranlates ItemSerializer::Type to corresponding ItemSerializer TypeId
		/// </summary>
		/// <param name="type"> ItemSerializer::Type </param>
		/// <returns> TypeId </returns>
		inline TypeId ItemSerializer::TypeToTypeId(Type type) {
			static const TypeId* const TYPE_IDS = []() -> const TypeId* {
			static const uint8_t TYPE_COUNT = static_cast<uint8_t>(Type::SERIALIZER_TYPE_COUNT);
				static TypeId typeIds[TYPE_COUNT];
				for (uint8_t i = 0; i < TYPE_COUNT; i++)
					typeIds[i] = TypeId::Of<void>();
				typeIds[static_cast<uint8_t>(Type::BOOL_VALUE)] = TypeId::Of<ValueSerializer<bool>>();
				typeIds[static_cast<uint8_t>(Type::CHAR_VALUE)] = TypeId::Of<ValueSerializer<char>>();
				typeIds[static_cast<uint8_t>(Type::SCHAR_VALUE)] = TypeId::Of<ValueSerializer<signed char>>();
				typeIds[static_cast<uint8_t>(Type::UCHAR_VALUE)] = TypeId::Of<ValueSerializer<unsigned char>>();
				typeIds[static_cast<uint8_t>(Type::WCHAR_VALUE)] = TypeId::Of<ValueSerializer<wchar_t>>();
				typeIds[static_cast<uint8_t>(Type::SHORT_VALUE)] = TypeId::Of<ValueSerializer<short>>();
				typeIds[static_cast<uint8_t>(Type::USHORT_VALUE)] = TypeId::Of<ValueSerializer<unsigned short>>();
				typeIds[static_cast<uint8_t>(Type::INT_VALUE)] = TypeId::Of<ValueSerializer<int>>();
				typeIds[static_cast<uint8_t>(Type::UINT_VALUE)] = TypeId::Of<ValueSerializer<unsigned int>>();
				typeIds[static_cast<uint8_t>(Type::LONG_VALUE)] = TypeId::Of<ValueSerializer<long>>();
				typeIds[static_cast<uint8_t>(Type::ULONG_VALUE)] = TypeId::Of<ValueSerializer<unsigned long>>();
				typeIds[static_cast<uint8_t>(Type::LONG_LONG_VALUE)] = TypeId::Of<ValueSerializer<long long>>();
				typeIds[static_cast<uint8_t>(Type::ULONG_LONG_VALUE)] = TypeId::Of<ValueSerializer<unsigned long long>>();
				typeIds[static_cast<uint8_t>(Type::FLOAT_VALUE)] = TypeId::Of<ValueSerializer<float>>();
				typeIds[static_cast<uint8_t>(Type::DOUBLE_VALUE)] = TypeId::Of<ValueSerializer<double>>();
				typeIds[static_cast<uint8_t>(Type::VECTOR2_VALUE)] = TypeId::Of<ValueSerializer<Vector2>>();
				typeIds[static_cast<uint8_t>(Type::VECTOR3_VALUE)] = TypeId::Of<ValueSerializer<Vector3>>();
				typeIds[static_cast<uint8_t>(Type::VECTOR4_VALUE)] = TypeId::Of<ValueSerializer<Vector4>>();
				typeIds[static_cast<uint8_t>(Type::MATRIX2_VALUE)] = TypeId::Of<ValueSerializer<Matrix2>>();
				typeIds[static_cast<uint8_t>(Type::MATRIX3_VALUE)] = TypeId::Of<ValueSerializer<Matrix3>>();
				typeIds[static_cast<uint8_t>(Type::MATRIX4_VALUE)] = TypeId::Of<ValueSerializer<Matrix4>>();
				typeIds[static_cast<uint8_t>(Type::STRING_VIEW_VALUE)] = TypeId::Of<ValueSerializer<std::string_view>>();
				typeIds[static_cast<uint8_t>(Type::WSTRING_VIEW_VALUE)] = TypeId::Of<ValueSerializer<std::wstring_view>>();
				typeIds[static_cast<uint8_t>(Type::OBJECT_PTR_VALUE)] = TypeId::Of<ObjectReferenceSerializer>();
				typeIds[static_cast<uint8_t>(Type::SERIALIZER_LIST)] = TypeId::Of<SerializerList>();
				return typeIds;
			}();
			return (type < Type::SERIALIZER_TYPE_COUNT) ? TYPE_IDS[static_cast<uint8_t>(type)] : TypeId::Of<void>();
		}

		/// <summary>
		/// Type-casts the serializer to ValueSerializer<ValueType> and retrieves value
		/// Note: Will crash if the serializer is not of a correct type
		/// </summary>
		/// <typeparam name="ValueType"> ValueSerializer's ValueType </typeparam>
		template<typename ValueType>
		inline SerializedObject::operator ValueType()const {
			const ValueSerializer<ValueType>* serializer = As<ValueSerializer<ValueType>>();
#ifndef NDEBUG
			// Make sure the user has some idea why we crashed:
			assert(serializer != nullptr);
#endif
			return serializer->Get(TargetAddr());
		}

		/// <summary>
		/// Type-casts the serializer to ValueSerializer<ValueType> and sets the value
		/// Note: Will crash if the serializer is not of a correct type
		/// </summary>
		/// <typeparam name="ValueType"> ValueSerializer's ValueType </typeparam>
		/// <param name="value"> Value to set </param>
		template<typename ValueType>
		inline void SerializedObject::operator=(const ValueType& value)const {
			const ValueSerializer<ValueType>* serializer = As<ValueSerializer<ValueType>>();
#ifndef NDEBUG
			// Make sure the user has some idea why we crashed:
			assert(serializer != nullptr);
#endif
			serializer->Set(value, TargetAddr());
		}

		/// <summary>
		/// Type-casts the serializer to SerializerList and invokes GetFields() with the callback and the TargetAddr
		/// Note: Will crash if the serializer is not of a correct type
		/// </summary>
		/// <typeparam name="RecordCallback"> Anything, that can be called with a Serialization::SerializedObject as an argument  </typeparam>
		/// <param name="callback"> Callback for SerializerList::GetFields() </param>
		template<typename RecordCallback>
		inline void SerializedObject::GetFields(const RecordCallback& callback)const {
			const SerializerList* serializer = As<SerializerList>();
#ifndef NDEBUG
			// Make sure the user has some idea why we crashed:
			assert(serializer != nullptr);
#endif
			void(*wrappedCallback)(const RecordCallback*, SerializedObject) = [](const RecordCallback* call, SerializedObject object) { (*call)(object); };
			serializer->GetFields(Callback<SerializedObject>(wrappedCallback, &callback), TargetAddr());
		}


		/** Here are all ValueSerializer the engine backend is aware of */

		/// <summary> bool value serializer </summary>
		typedef ValueSerializer<bool> BoolSerializer;
		static_assert(BoolSerializer::SerializerType() == ItemSerializer::Type::BOOL_VALUE);
		static_assert(std::is_same_v<BoolSerializer::From<bool>::TargetType, bool>);

		/// <summary> char value serializer </summary>
		typedef ValueSerializer<char> CharSerializer;
		static_assert(CharSerializer::SerializerType() == ItemSerializer::Type::CHAR_VALUE);
		static_assert(std::is_same_v<CharSerializer::From<char>::TargetType, char>);

		/// <summary> signed char value serializer </summary>
		typedef ValueSerializer<signed char> ScharSerializer;
		static_assert(ScharSerializer::SerializerType() == ItemSerializer::Type::SCHAR_VALUE);

		/// <summary> unsigned char value serializer </summary>
		typedef ValueSerializer<unsigned char> UcharSerializer;
		static_assert(UcharSerializer::SerializerType() == ItemSerializer::Type::UCHAR_VALUE);

		/// <summary> wide char value serializer </summary>
		typedef ValueSerializer<wchar_t> WcharSerializer;
		static_assert(WcharSerializer::SerializerType() == ItemSerializer::Type::WCHAR_VALUE);

		/// <summary> short value serializer </summary>
		typedef ValueSerializer<short> ShortSerializer;
		static_assert(ShortSerializer::SerializerType() == ItemSerializer::Type::SHORT_VALUE);
		static_assert(ValueSerializer<signed short>::SerializerType() == ItemSerializer::Type::SHORT_VALUE);

		/// <summary> unsigned short value serializer </summary>
		typedef ValueSerializer<unsigned short> UshortSerializer;
		static_assert(UshortSerializer::SerializerType() == ItemSerializer::Type::USHORT_VALUE);

		/// <summary> int value serializer </summary>
		typedef ValueSerializer<int> IntSerializer;
		static_assert(IntSerializer::SerializerType() == ItemSerializer::Type::INT_VALUE);
		static_assert(ValueSerializer<signed int>::SerializerType() == ItemSerializer::Type::INT_VALUE);

		/// <summary> unsigned int value serializer </summary>
		typedef ValueSerializer<unsigned int> UintSerializer;
		static_assert(UintSerializer::SerializerType() == ItemSerializer::Type::UINT_VALUE);

		/// <summary> long value serializer </summary>
		typedef ValueSerializer<long> LongSerializer;
		static_assert(LongSerializer::SerializerType() == ItemSerializer::Type::LONG_VALUE);
		static_assert(ValueSerializer<signed long>::SerializerType() == ItemSerializer::Type::LONG_VALUE);

		/// <summary> unsigned long value serializer </summary>
		typedef ValueSerializer<unsigned long> UlongSerializer;
		static_assert(UlongSerializer::SerializerType() == ItemSerializer::Type::ULONG_VALUE);

		/// <summary> long long value serializer </summary>
		typedef ValueSerializer<long long> LongLongSerializer;
		static_assert(LongLongSerializer::SerializerType() == ItemSerializer::Type::LONG_LONG_VALUE);
		static_assert(ValueSerializer<signed long long>::SerializerType() == ItemSerializer::Type::LONG_LONG_VALUE);

		/// <summary> unsigned long value serializer </summary>
		typedef ValueSerializer<unsigned long long> UlongLongSerializer;
		static_assert(UlongLongSerializer::SerializerType() == ItemSerializer::Type::ULONG_LONG_VALUE);

		/// <summary> 32 bit (single precision) floating point value serializer </summary>
		typedef ValueSerializer<float> FloatSerializer;
		static_assert(FloatSerializer::SerializerType() == ItemSerializer::Type::FLOAT_VALUE);

		/// <summary> 64 bit (double precision) floating point value serializer </summary>
		typedef ValueSerializer<double> DoubleSerializer;
		static_assert(DoubleSerializer::SerializerType() == ItemSerializer::Type::DOUBLE_VALUE);

		/// <summary> 2D vector value serializer </summary>
		typedef ValueSerializer<Vector2> Vector2Serializer;
		static_assert(Vector2Serializer::SerializerType() == ItemSerializer::Type::VECTOR2_VALUE);

		/// <summary> 3D vector value serializer </summary>
		typedef ValueSerializer<Vector3> Vector3Serializer;
		static_assert(Vector3Serializer::SerializerType() == ItemSerializer::Type::VECTOR3_VALUE);

		/// <summary> 4D vector value serializer </summary>
		typedef ValueSerializer<Vector4> Vector4Serializer;
		static_assert(Vector4Serializer::SerializerType() == ItemSerializer::Type::VECTOR4_VALUE);

		/// <summary> 2D matrix value serializer </summary>
		typedef ValueSerializer<Matrix2> Matrix2Serializer;
		static_assert(Matrix2Serializer::SerializerType() == ItemSerializer::Type::MATRIX2_VALUE);

		/// <summary> 3D matrix value serializer </summary>
		typedef ValueSerializer<Matrix3> Matrix3Serializer;
		static_assert(Matrix3Serializer::SerializerType() == ItemSerializer::Type::MATRIX3_VALUE);

		/// <summary> 4D matrix value serializer </summary>
		typedef ValueSerializer<Matrix4> Matrix4Serializer;
		static_assert(Matrix4Serializer::SerializerType() == ItemSerializer::Type::MATRIX4_VALUE);

		/// <summary> 
		/// String value serializer 
		/// (we use std::string_view to reduce unnecessary allocations, but that means that manual getter and setter become more or less mandatory) 
		/// </summary>
		typedef ValueSerializer<std::string_view> StringViewSerializer;
		static_assert(StringViewSerializer::SerializerType() == ItemSerializer::Type::STRING_VIEW_VALUE);

		/// <summary> 
		/// Wide String value serializer 
		/// (we use std::wstring_view to reduce unnecessary allocations, but that means that manual getter and setter become more or less mandatory) 
		/// </summary>
		typedef ValueSerializer<std::wstring_view> WideStringViewSerializer;
		static_assert(WideStringViewSerializer::SerializerType() == ItemSerializer::Type::WSTRING_VIEW_VALUE);

		/// <summary> 32 bit integer value serializer </summary>
		typedef ValueSerializer<int32_t> Int32Serializer;
		static_assert(Int32Serializer::SerializerType() != ItemSerializer::Type::ERROR_TYPE);

		/// <summary> 32 bit unsigned integer value serializer </summary>
		typedef ValueSerializer<uint32_t> Uint32Serializer;
		static_assert(Uint32Serializer::SerializerType() != ItemSerializer::Type::ERROR_TYPE);

		/// <summary> 64 bit integer value serializer </summary>
		typedef ValueSerializer<int64_t> Int64Serializer;
		static_assert(Int64Serializer::SerializerType() != ItemSerializer::Type::ERROR_TYPE);

		/// <summary> 64 bit unsigned integer value serializer </summary>
		typedef ValueSerializer<uint64_t> Uint64Serializer;
		static_assert(Uint64Serializer::SerializerType() != ItemSerializer::Type::ERROR_TYPE);

		/// <summary> size_t value serializer </summary>
		typedef ValueSerializer<size_t> SizeSerializer;
		static_assert(SizeSerializer::SerializerType() != ItemSerializer::Type::ERROR_TYPE);

		// Check values for resource reference serializers:
		static_assert(std::is_base_of_v<ObjectReferenceSerializer, ValueSerializer<Object*>>);
		static_assert(ValueSerializer<Object*>::SerializerType() == ItemSerializer::Type::OBJECT_PTR_VALUE);
		static_assert(ValueSerializer<BuiltInTypeRegistrator*>::SerializerType() == ItemSerializer::Type::OBJECT_PTR_VALUE);
	}
}
#pragma warning(default: 4250)
