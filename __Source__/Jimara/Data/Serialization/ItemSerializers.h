#pragma once
#include "../../Math/Math.h"
#include "../TypeRegistration/TypeRegistartion.h"
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





		/// <summary>
		/// Parent class of all item/object/resource serializers.
		/// </summary>
		class ItemSerializer : public virtual Object {
		protected:
			/// <summary> This should return what type of a serializer we're dealing with (Engine internals will only acknowledge SerializerList and ValueSerializer<>) </summary>
			virtual TypeId GetSerializerFamily()const = 0;

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
			inline TypeId SerializerFamily()const {
				TypeId id = GetSerializerFamily();
#ifndef NDEBUG
				// Protection against 'Fake values'
				assert(id.CheckType(this));
#endif
				return id;
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
			template<typename TargetAddrType>
			class Of;

		private:
			// Target type name
			const std::string m_name;

			// Target hint (editor helper texts on hover and what not)
			const std::string m_hint;

			// Serializer attributes
			const std::vector<Reference<const Object>> m_attributes;
		};


		/// <summary>
		/// Pair of an ItemSerializer and corresponding target address
		/// </summary>
		class SerializedObject {
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
				: m_serializer(serializer), m_targetAddr((void*)target) {}

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
		};

		/// <summary>
		/// ItemSerializer that knows how to interpret user data
		/// </summary>
		/// <typeparam name="TargetAddrType"></typeparam>
		template<typename TargetAddrType>
		class ItemSerializer::Of : public virtual ItemSerializer {
		public:
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
		/// </summary>
		class SerializerList : public virtual ItemSerializer {
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
			/// <summary> TypeId::Of<SerializerList>() </summary>
			virtual TypeId GetSerializerFamily()const override { return TypeId::Of<SerializerList>(); }

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
		/// Note: Even if this is a template, only the set amount of types are known by the engine and all of those have separate type definitions down below.
		/// </summary>
		/// <typeparam name="ValueType"> Scalar/Vector value, as well as some other simple/built-in classes like string </typeparam>
		template<typename ValueType>
		class ValueSerializer : public virtual ItemSerializer {
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

		protected:
			/// <summary> TypeId::Of<ValueSerializer<ValueType>>() </summary>
			virtual TypeId GetSerializerFamily()const final override { return TypeId::Of<ValueSerializer<ValueType>>(); }

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


		/** Here are all ValueSerializer the engine backend is aware of */

		/// <summary> Boolean value serializer </summary>
		typedef ValueSerializer<bool> BoolSerializer;

		/// <summary> Integer value serializer </summary>
		typedef ValueSerializer<int> IntSerializer;

		/// <summary> Unsigned integer value serializer </summary>
		typedef ValueSerializer<unsigned int> UintSerializer;

		/// <summary> 32 bit integer value serializer </summary>
		typedef ValueSerializer<int32_t> Int32Serializer;

		/// <summary> 32 bit unsigned integer value serializer </summary>
		typedef ValueSerializer<uint32_t> Uint32Serializer;

		/// <summary> 64 bit integer value serializer </summary>
		typedef ValueSerializer<int64_t> Int64Serializer;

		/// <summary> 64 bit unsigned integer value serializer </summary>
		typedef ValueSerializer<uint64_t> Uint64Serializer;

		/// <summary> 32 bit (single precision) floating point value serializer </summary>
		typedef ValueSerializer<float> FloatSerializer;

		/// <summary> 64 bit (double precision) floating point value serializer </summary>
		typedef ValueSerializer<double> DoubleSerializer;

		/// <summary> 2D vector value serializer </summary>
		typedef ValueSerializer<Vector2> Vector2Serializer;

		/// <summary> 3D vector value serializer </summary>
		typedef ValueSerializer<Vector3> Vector3Serializer;

		/// <summary> 4D vector value serializer </summary>
		typedef ValueSerializer<Vector4> Vector4Serializer;

		/// <summary> 
		/// String value serializer 
		/// (we use std::string_view to reduce unnecessary allocations, but that means that manual getter and setter become more or less mandatory) 
		/// </summary>
		typedef ValueSerializer<const std::string_view> StringViewSerializer;

		/// <summary> 
		/// Wide String value serializer 
		/// (we use std::wstring_view to reduce unnecessary allocations, but that means that manual getter and setter become more or less mandatory) 
		/// </summary>
		typedef ValueSerializer<const std::wstring_view> WideStringViewSerializer;
	}
}
#pragma warning(default: 4250)
