#pragma once
#include "../../Core/Object.h"
#include "../../Core/Function.h"
#include "../../Core/Collections/Stacktor.h"
#include <type_traits>
#include <typeindex>
#include <algorithm>
#include <vector>

/**
Here are a few macros for defining and implementing projet-wide type registries:
Generally speaking, we will frequently encounter a case when we need to access some type definitions 
or make sure some global objects are initialized when we have our game up and running; 
for example, Editor and scene loader will both need to know all component and other resource types available through the solution, 
even if they are not directly referenced from within the code.
"Type registrator classes", alongside corresponding pre-build events are designed to resolve that issue.

For integration, one should follow these steps:

0. Define type registrator class:
	___________________________________
	/// "OurProjectTypeRegistry.h":
	#include "path/to/TypeRegistartion.h"
	namespace OurProjectNamespace {
		JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(OurProjectTypeRegistry);
	}

1. Add pre-build event (for msvs) or a pre-build makefile rule (linux):
	python(3) "path/to/Jimara/repository/__Scripts__/jimara_implement_type_registrator.py" "path/to/project/source" "OurProjectNamespace::OurProjectTypeRegistry" "header/to/generate.impl.h"

2. Include generated header into project via compiled source:
	___________________________________
	/// "OurProjectTypeRegistry.cpp":
	#include "OurProjectTypeRegistry.h"
	#include "header/to/generate.impl.h"

3. Make sure, "header/to/generate.impl.h" is included in .gitignore and soes not mess up your commit history :);
4. Make sure, OurProjectTypeRegistry is the only type registration class used by your project to avoid unnecesssary complications;

5. For any type you wish to be included in registry, write:
	___________________________________
	/// "OurClassType.h":
	#include "OurProjectTypeRegistry.h"
	namespace OurProjectNamespace {
		JIMARA_REGISTER_TYPE(OurProjectNamespace::OurClassType);

		class OurClassType {
			// Definition of your class...
		};
	}
	namespace Jimara {
		// Override TypeIdDetails for your class if you want to, let's say, expose parent classes of your class to TypeId, 
		// or you want to add some attributes of your type or do some actions when your class gets regsitered.
		// Specialisation have to be defined in header, but the implementation can, naturally, go wherever you want.
		// Feel free to read through TypeIdDetails for further details;
		template<> TypeIdDetails::__SOME_CALLBACKS_FROM_TypeIdDetails<OurProjectNamespace::OurClassType>(CALLBACK_ARGS);
		// More specialisations for TypeIdDetails if needed...
	}

6. Once we've done these, "header/to/generate.impl.h" will automagically request TypeId registration tokens throught thre lifecycle of the OurProjectNamespace::OurProjectTypeRegistry instance;
7. In order to activate type registrations, simply define "Jimara::Reference<OurProjectNamespace::OurProjectTypeRegistry> reference = OurProjectNamespace::OurProjectTypeRegistry::Instance();" 
	and keep it alive while the side effects between "RegisterType" and "UnregisterType" calls are needed (ei, create one in main and keep it there till the program quits in 99% of the cases).
*/

/// <summary> Defines type registartor class </summary>
#define JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(TypeRegistrationClass) \
	class TypeRegistrationClass : public virtual Jimara::Object { \
	public: \
		/** Singleton instance of the registartor type (as long as anyone's holding this instance, the types will stay registered) */ \
		static Reference<TypeRegistrationClass> Instance(); \
	private: \
		std::vector<Reference<const Object>> m_typeRegistrationTokens; \
		TypeRegistrationClass(); \
		virtual void OnOutOfScope()const final override; \
	}

/// <summary> 
/// Lets the pre-build event know that given class should be included in project-wide type registration
/// Notes:
///		0. RegisteredClassType body should contain JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS definition;
///		1. RegisteredClassType implementation has to include JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS;
///		2. In case JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS is private, make sure to befrend the project-wide registrator type (just add 'friend class YourProjectsRegistrator');
///		3. RegisteredClassType is adviced to be the "full name" of the class, including all the namespaces (ei JIMARA_REGISTER_TYPE(Namespace::Class) instead of JIMARA_REGISTER_TYPE(Class)) 
///			to make sure the registrator implementation can access it without complications.
/// </summary>
#define JIMARA_REGISTER_TYPE(RegisteredClassType)


namespace Jimara {
	/// <summary>
	/// Basic information about types:
	/// Note: Specialize TypeId::ParentTypes::Of<> per type to 'register' information about inheritance (you can make that information junk, but I hope you won't)
	/// </summary>
	class TypeId {
	private:
		// Type name
		std::string_view m_typeName = "void";

		// Type information
		const std::type_info* m_typeInfo = &typeid(void);

		// Type check function
		typedef bool(*CheckTypeFn)(const Object*);
		CheckTypeFn m_checkType = [](const Object*) -> bool { return true; };

		// Inheritance info getter
		typedef void(*ParentTypeGetter)(const Callback<TypeId>&);
		ParentTypeGetter m_getParentTypes;

		// Attributes getter
		typedef void(*TypeAttributeGetter)(const Callback<const Object*>&);
		TypeAttributeGetter m_getTypeAttributes;

		// Registration callbacks
		typedef void(*RegistrationCallback)();
		typedef void(*RegistrationCallbackGetter)(RegistrationCallback&, RegistrationCallback&);
		RegistrationCallbackGetter m_registrationCallbackGetter;

		// Type check function for classes
		template<typename Type>
		inline static constexpr typename std::enable_if<std::is_class<Type>::value, CheckTypeFn>::type CheckType() {
			return [](const Object* object)->bool { return dynamic_cast<const Type*>(object) != nullptr; };
		}

		// Type check function for non-classes
		template<typename Type>
		inline static constexpr typename std::enable_if<!std::is_class<Type>::value, CheckTypeFn>::type CheckType() {
			return [](const Object*) { return std::is_same<Type, void>::value; };
		}

		// Constructor (a private one, to prevent incorrect assignment)
		inline constexpr TypeId(const std::string_view& typeName, CheckTypeFn checkType, const std::type_info& typeInfo,
			const ParentTypeGetter& getParentTypes, const TypeAttributeGetter& getTypeAttributes, const RegistrationCallbackGetter& registrationCallbackGetter)
			: m_typeName(typeName), m_checkType(checkType), m_typeInfo(&typeInfo)
			, m_getParentTypes(getParentTypes), m_getTypeAttributes(getTypeAttributes), m_registrationCallbackGetter(registrationCallbackGetter) {}

		// In order to get type name, we'll need some random string that contains it only once; this will generally suffice:
		template<typename Type>
		inline static constexpr std::string_view TemplateSignature() {
#ifdef __clang__
			return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
			return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
			return __FUNCSIG__;
#else
#error "Jimara::TypeId - Unknown compiler type!"
			return "";
#endif
		}

	public:
		/// <summary> Default constructor </summary>
		inline constexpr TypeId();

		/// <summary> Copy-Constructor </summary>
		inline constexpr TypeId(const TypeId&) = default;

		/// <summary> Copy-Assignment </summary>
		inline constexpr TypeId& operator=(const TypeId&) = default;

		/// <summary> Type name (full path) </summary>
		inline constexpr std::string_view Name()const { return m_typeName; }

		/// <summary> Type index </summary>
		inline std::type_index TypeIndex()const { return *m_typeInfo; }

		/// <summary>
		/// Iterates over the parent types 
		/// Notes: 
		///		0. TypeIdDetails::GetParentTypesOf<> has to be specialized for the type for this to work properly;
		///		1. This call will not recurse; only immediate parents that would be returned by calling TypeIdDetails::GetParentTypesOf<> will appear here.
		/// </summary>
		/// <param name="reportParentType"> Each parent type will be reported through this callback </param>
		inline void GetParentTypes(const Callback<TypeId>& reportParentType)const { m_getParentTypes(reportParentType); }

		/// <summary>
		/// Iterates over the parent types 
		/// Notes: 
		///		0. TypeIdDetails::GetParentTypesOf<> has to be specialized for the type for this to work properly;
		///		1. This call will not recurse; only immediate parents that would be returned by calling TypeIdDetails::GetParentTypesOf<> will appear here.
		/// </summary>
		/// <typeparam name="CallbackType"> Any callback function, as long as it accepts TypeId as an argument </typeparam>
		/// <param name="reportParentType"> Each parent type will be reported through this callback </param>
		template<typename CallbackType>
		inline void IterateParentTypes(const CallbackType& reportParentType)const {
			void(*callback)(const CallbackType*, TypeId) = [](const CallbackType* call, TypeId parentType) { (*call)(parentType); };
			GetParentTypes(Callback<TypeId>(callback, &reportParentType));
		}

		/// <summary>
		/// Checks(recursively), if thre recursive set of parents of this type includes other
		/// Note: Call relies on TypeIdDetails::GetParentTypesOf<>. This means that correctness of this function fully relies on 
		///		the designer of the class to correctly specialize TypeIdDetails, which is neither required, nor guaranteed. 
		///		Therefore, this one should be used with a certain amount of caution.
		/// </summary>
		/// <param name="other"> Parent type </param>
		/// <returns> True, if other is a parent type of this </returns>
		bool IsDerivedFrom(const TypeId& other)const;

		/// <summary>
		/// Iterates over arbitrary type attribute objects associated with the type
		/// Notes:
		///		0. You can specialize TypeIdDetails::GetTypeAttributesOf<> and TypeId::Of<>().GetAttributes() will be able to report said attributes;
		///		1. Attributes can be arbitrary objects, any behaviour and logic beyond that is user-defined. One example could be something like attaching a serializer to the target type;
		///		2. Once again, this meshod only invokes TypeIdDetails::GetTypeAttributesOf<> and does not look at the attributes of the parent types.
		/// </summary>
		/// <param name="reportTypeAttributes"> Each attribute object of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
		inline void GetAttributes(const Callback<const Object*>& reportTypeAttributes)const { return m_getTypeAttributes(reportTypeAttributes); }

		/// <summary>
		/// Iterates over arbitrary type attribute objects associated with the type
		/// Notes:
		///		0. You can specialize TypeIdDetails::GetTypeAttributesOf<> and TypeId::Of<>().GetAttributes() will be able to report said attributes;
		///		1. Attributes can be arbitrary objects, any behaviour and logic beyond that is user-defined. One example could be something like attaching a serializer to the target type;
		///		2. Once again, this meshod only invokes TypeIdDetails::GetTypeAttributesOf<> and does not look at the attributes of the parent types.
		/// </summary>
		/// <typeparam name="CallbackType"> Any callback function, as long as it accepts const Object* as an argument </typeparam>
		/// <param name="reportTypeAttributes"> Each attribute object of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
		template<typename CallbackType>
		inline void IterateAttributes(const CallbackType& reportTypeAttributes)const {
			void(*callback)(const CallbackType*, const Object*) = [](const CallbackType* call, const Object* attribute) { (*call)(attribute); };
			GetAttributes(Callback<const Object*>(callback, &reportTypeAttributes));
		}

		/// <summary>
		/// Searches for an attribute of given type
		/// </summary>
		/// <typeparam name="Type"> Type of an attribute to search for </typeparam>
		/// <param name="includeParentAttributes"> If true, parents types will be seached for recursively </param>
		/// <returns> Attribute, if found one, nullptr otherwise </returns>
		template<typename Type>
		inline const Type* FindAttributeOfType(bool includeParentAttributes = false)const {
			const Type* result = nullptr;
			IterateAttributes([&](const Object* attribute) {
				if (result == nullptr)
					result = dynamic_cast<const Type*>(attribute);
				});
			if (result == nullptr && includeParentAttributes)
				IterateParentTypes([&](TypeId parentId) {
				if (result == nullptr)
					result = parentId.FindAttributeOfType<Type>(true);
					});
			return result;
		}

		/// <summary>
		/// Checks if the object is derived from this type
		/// </summary>
		/// <param name="object"> Object to check </param>
		/// <returns> True, if the object is of this type </returns>
		inline bool CheckType(const Object* object)const { return m_checkType(object); }

		/// <summary> Compares two TypeId objects (returns true, if this is 'less than' other) </summary>
		inline bool operator<(const TypeId& other)const { return TypeIndex() < other.TypeIndex(); }

		/// <summary> Compares two TypeId objects (returns true, if this is 'less than' or equal to other) </summary>
		inline bool operator<=(const TypeId& other)const { return TypeIndex() <= other.TypeIndex(); }

		/// <summary> Compares two TypeId objects (returns true, if this is equal to other) </summary>
		inline bool operator==(const TypeId& other)const { return TypeIndex() == other.TypeIndex(); }

		/// <summary> Compares two TypeId objects (returns true, if this is not equal to other) </summary>
		inline bool operator!=(const TypeId& other)const { return TypeIndex() != other.TypeIndex(); }

		/// <summary> Compares two TypeId objects (returns true, if this is 'greater than' or equal to other) </summary>
		inline bool operator>=(const TypeId& other)const { return TypeIndex() >= other.TypeIndex(); }

		/// <summary> Compares two TypeId objects (returns true, if this is 'greater than' other) </summary>
		inline bool operator>(const TypeId& other)const { return TypeIndex() > other.TypeIndex(); }

		/// <summary>
		/// Generates TypeId for given type
		/// </summary>
		/// <typeparam name="Type"> Type to generate TypeId of </typeparam>
		/// <returns> TypeId for Type </returns>
		template<typename Type>
		inline static constexpr TypeId Of();

		/// <summary>
		/// Registers type in global registry 
		/// (enables retrieving type information from std::type_index)
		/// </summary>
		/// <returns> "Registration token"; While this one is 'alive', TypeId record will be kept inside the registry </returns>
		Reference<Object> Register()const;

		/// <summary>
		/// Searches for the TypeId record within the global registry
		/// Note: Will fail, unless the type was previously registered using TypeId::Register() call and it's registration token is still alive
		/// </summary>
		/// <param name="typeInfo"> C++ type info to search our TypeId with </param>
		/// <param name="result"> If successful, TypeId will be stored here </param>
		/// <returns> True, if typeInfo record is found inside the global registry; false otherwise </returns>
		static bool Find(const std::type_info& typeInfo, TypeId& result);

		/// <summary>
		/// Searches for the TypeId record within the global registry
		/// Note: Will fail, unless the type was previously registered using TypeId::Register() call and it's registration token is still alive
		/// </summary>
		/// <param name="typeName"> Type name (full namespace path) </param>
		/// <param name="result"> If successful, TypeId will be stored here </param>
		/// <returns> True, if typeName record is found inside the global registry; false otherwise </returns>
		static bool Find(const std::string_view& typeName, TypeId& result);

		/// <summary>
		/// Iterates over all registered types
		/// Notes: 
		///		0. reportType callback can do anything, as long as no type gets registered or gets erased from the global registry as a side effect.
		///		Ignoring that will more than likely result in a crash;
		///		1. This routine will report only types that have been registered with TypeId::Register() call, while their "registration token" stays alive;
		///		2. Type registrators invoke TypeId::Register() for each class marked with JIMARA_REGISTER_TYPE, any type that has it, will also be reported,
		///		as long as corresponding Type registry instance is active.
		/// </summary>
		/// <param name="reportType"> Each globally registered type will be reported through  </param>
		static void GetRegisteredTypes(const Callback<TypeId>& reportType);

		/// <summary>
		/// Iterates over all registered types
		/// Notes: 
		///		0. reportType callback can do anything, as long as no type gets registered or gets erased from the global registry as a side effect.
		///		Ignoring that will more than likely result in a crash;
		///		1. This routine will report only types that have been registered with TypeId::Register() call, while their "registration token" stays alive;
		///		2. Type registrators invoke TypeId::Register() for each class marked with JIMARA_REGISTER_TYPE, any type that has it, will also be reported,
		///		as long as corresponding Type registry instance is active.
		/// </summary>
		/// <typeparam name="CallbackType"> Anything callable, that will accept TypeId as an argument </typeparam>
		/// <param name="reportType"> Each globally registered type will be reported through  </param>
		template<typename CallbackType>
		inline static void IterateRegisteredTypes(const CallbackType& reportType) {
			void(*callback)(const CallbackType*, TypeId) = [](const CallbackType* report, TypeId typeId) { (*report)(typeId); };
			GetRegisteredTypes(Callback<TypeId>(callback, &reportType));
		}
	};

	/// <summary>
	/// TypeId has to retrieve some information about types (like parents or attributes), as well as invoke a bounch of registration callbacks; 
	/// if a class wants to sopport those, it is free to override callbacks defined inside tris class
	/// </summary>
	class TypeIdDetails {
	private:
		// Only TypeId can directly invoke the callbacks defined here
		friend class TypeId;

		/// <summary>
		/// Callbacks from TypeIdDetails rely on this class by default;
		/// If you wish to override each of those togather, feel free to simply define your implementation for this structure.
		/// Note: Under most circumstances, overloading individual callbacks will likely be more convenient; However, for some templates, this might be the only way...
		/// </summary>
		/// <typeparam name="Type"> Type, the 'default behaviour' applies to </typeparam>
		template<typename Type>
		struct TypeDetails {
			/// <summary>
			/// Default behaviour of TypeIdDetails::GetParentTypesOf<Type>(reportParentType) call
			/// </summary>
			/// <param name="reportParentType"> Each parent of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
			inline static void GetParentTypes(const Callback<TypeId>& reportParentType) { }

			/// <summary>
			/// Default behaviour of TypeIdDetails::GetTypeAttributesOf<Type>(reportTypeAttributes) call
			/// </summary>
			/// <param name="reportTypeAttributes"> Each attribute object of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
			inline static void GetTypeAttributes(const Callback<const Object*>& reportTypeAttributes) { }

			/// <summary> Default behaviour of TypeIdDetails::OnRegisterType<Type>() call </summary>
			inline static void OnRegisterType() {}

			/// <summary> Default behaviour of TypeIdDetails::OnUnregisterType<Type>() call </summary>
			inline static void OnUnregisterType() {}
		};

		/// <summary>
		/// Defines behaviour of TypeId::Of<Type>().GetParentTypes();
		/// Note: Override this to let the engine know about the parent classes/intefaces of given type
		/// </summary>
		/// <typeparam name="Type"> Type, to report parent types of </typeparam>
		/// <param name="reportParentType"> Each parent of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
		template<typename Type>
		inline static void GetParentTypesOf(const Callback<TypeId>& reportParentType) { TypeDetails<Type>::GetParentTypes(reportParentType); }

		/// <summary>
		/// Defines behaviour of TypeId::Of<Type>().GetAttributes();
		/// Note: Override this to define any number of attributes of a given type
		/// </summary>
		/// <typeparam name="Type"> Type, to report attribute objects of </typeparam>
		/// <param name="reportTypeAttributes"> Each attribute object of Type should be reported by invoking this callback (this approach enables zero-allocation iteration) </param>
		template<typename Type>
		inline static void GetTypeAttributesOf(const Callback<const Object*>& reportTypeAttributes) { TypeDetails<Type>::GetTypeAttributes(reportTypeAttributes); }

		/// <summary>
		/// Invoked, when TypeId::Of<Type>().Register() creates a registration token
		/// Notes: 
		///		0. Override this, if you want to do something specific when a type registration token is created;
		///		1. It is not allowed to request/remove registration of another type from this callback; doing so will likely result in a crash.
		/// </summary>
		/// <typeparam name="Type"> Type, this callback targets </typeparam>
		template<typename Type>
		inline static void OnRegisterType() { TypeDetails<Type>::OnRegisterType(); }

		/// <summary>
		/// Invoked, when registration token created by TypeId::Of<Type>().Register() goes out of scope
		/// Notes: 
		///		0. Override this, if you want to do something specific when a type registration token is destroyed;
		///		1. It is not allowed to request/remove registration of another type from this callback; doing so will likely result in a crash.
		/// </summary>
		/// <typeparam name="Type"> Type, this callback targets </typeparam>
		template<typename Type>
		inline static void OnUnregisterType() { TypeDetails<Type>::OnUnregisterType(); }
	};

	/// <summary> Default constructor </summary>
	inline constexpr TypeId::TypeId()
		: m_getParentTypes(TypeIdDetails::GetParentTypesOf<void>)
		, m_getTypeAttributes(TypeIdDetails::GetTypeAttributesOf<void>)
		, m_registrationCallbackGetter([](RegistrationCallback& onRegister, RegistrationCallback& onUnregister) {
		onRegister = TypeIdDetails::OnRegisterType<void>;
		onUnregister = TypeIdDetails::OnUnregisterType<void>;
			}) {}

	/// <summary>
	/// Generates TypeId for given type
	/// </summary>
	/// <typeparam name="Type"> Type to generate TypeId of </typeparam>
	/// <returns> TypeId for Type </returns>
	template<typename Type>
	inline constexpr TypeId TypeId::Of() {
		// Find type name definition:
		constexpr const std::string_view VOID_NAME = "void";
		constexpr const std::string_view VOID_SIGNATURE = TemplateSignature<void>();
		constexpr const size_t PREFFIX_LEN = VOID_SIGNATURE.find(VOID_NAME);
		constexpr const std::string_view SIGNATURE = TemplateSignature<Type>();
		constexpr const std::string_view TYPE_DEFINITION = SIGNATURE.substr(PREFFIX_LEN, SIGNATURE.length() + VOID_NAME.length() - VOID_SIGNATURE.length());

		// Remove 'class'/'struct' preffix to find real type name:
		constexpr const std::string_view CLASS_PREFFIX = "class ";
		constexpr const size_t CLASS_OFFSET = (TYPE_DEFINITION.find(CLASS_PREFFIX) == 0) ? CLASS_PREFFIX.length() : (size_t)0;
		constexpr const std::string_view STRUCT_PREFFIX = "struct ";
		constexpr const size_t STRUCT_OFFSET = (TYPE_DEFINITION.find(STRUCT_PREFFIX) == 0) ? STRUCT_PREFFIX.length() : (size_t)0;
		constexpr const std::string_view TYPE_NAME = TYPE_DEFINITION.substr(CLASS_OFFSET + STRUCT_OFFSET);

		// Type check function:
		constexpr const CheckTypeFn checkType = CheckType<Type>();

		// Type info:
		constexpr const std::type_info& typeInfo = typeid(Type);

		// Callbacks:
		constexpr const ParentTypeGetter getInheritance = [](const Callback<TypeId>& reportParentType) { 
			Jimara::TypeIdDetails::GetParentTypesOf<Type>(reportParentType); 
		};
		constexpr const TypeAttributeGetter getAttributes = [](const Callback<const Object*>& reportTypeAttributes) {
			Jimara::TypeIdDetails::GetTypeAttributesOf<Type>(reportTypeAttributes);
		};
		constexpr const RegistrationCallbackGetter registrationCallbackGetter = [](RegistrationCallback& onRegister, RegistrationCallback& onUnregister) {
			onRegister = TypeIdDetails::OnRegisterType<Type>;
			onUnregister = TypeIdDetails::OnUnregisterType<Type>;
		};

		// Create type id:
		return TypeId(TYPE_NAME, checkType, typeInfo, getInheritance, getAttributes, registrationCallbackGetter);
	}

	/// <summary>
	/// Collection of simultaneously registered TypeId objects
	/// </summary>
	class RegisteredTypeSet : public virtual Object {
	private:
		// Type set
		const std::vector<TypeId> m_types;

		// Constructor can only be accessed through "Current() call"
		inline RegisteredTypeSet(std::vector<TypeId>&& types) : m_types(std::move(types)) {}

	public:
		/// <summary> Number of registered types </summary>
		inline size_t Size()const { return m_types.size(); }

		/// <summary>
		/// Access to registered type by index
		/// </summary>
		/// <param name="index"> Type index </param>
		/// <returns> TypeId </returns>
		inline const TypeId& At(size_t index)const { return m_types[index]; }

		/// <summary>
		/// Access to registered type by index
		/// </summary>
		/// <param name="index"> Type index </param>
		/// <returns> TypeId </returns>
		inline const TypeId& operator[](size_t index)const { return At(index); }

		/// <summary>
		/// Set of currently registered types
		/// Notes: 
		///		0. Reference will change each time any type gets registered or removed; otherwise, it'll stay intact;
		///		1. Once constructed, this one is immutable, so no need to worry about stuff from TypeId::GetRegisteredTypes()
		/// </summary>
		/// <returns> Reference of current RegisteredTypeSet </returns>
		static Reference<RegisteredTypeSet> Current();
	};

	// expose RegisteredTypeSet parents:
	template<> inline void TypeIdDetails::GetParentTypesOf<RegisteredTypeSet>(const Callback<TypeId>& reportParentType) { reportParentType(TypeId::Of<Object>()); }

	// A few static asserts to make sure TypeId::Of<...>.Name() works as intended
	static_assert(TypeId::Of<void>().Name() == "void");
	static_assert(TypeId::Of<int>().Name() == "int");
	static_assert(TypeId::Of<float>().Name() == "float");
	static_assert(TypeId::Of<TypeId>().Name() == "Jimara::TypeId");
	static_assert(TypeId::Of<Object>().Name() == "Jimara::Object");


	/** 
	Built-in type registrator, all internal Jimara objects rely on 
	(Instance only needed to make accessing various type registries possible; the runtime will work fine without this one) 
	*/
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(BuiltInTypeRegistrator);

	/// <summary>
	/// Inheritance info of BuiltInTypeRegistrator
	/// </summary>
	/// <param name="reportParentType"> Each parent of Type will be reported by invoking this callback </param>
	template<> void TypeIdDetails::GetParentTypesOf<BuiltInTypeRegistrator>(const Callback<TypeId>& reportParentType);
}

namespace std {
	/// <summary>
	/// Standsrd library hash overload for TypeId
	/// </summary>
	template<>
	struct hash<Jimara::TypeId> {
		/// <summary>
		/// Standsrd library hash overload for TypeId
		/// </summary>
		/// <param name="typeId"> Type id </param>
		/// <returns> Hash of the identifier </returns>
		inline std::size_t operator()(const Jimara::TypeId& typeId)const {
			return std::hash<std::type_index>()(typeId.TypeIndex());
		}
	};
}
