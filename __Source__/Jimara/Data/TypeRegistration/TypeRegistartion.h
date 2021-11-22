#pragma once
#include "../../Core/Object.h"
#include "../../Core/Collections/Stacktor.h"
#include <type_traits>
#include <typeindex>
#include <algorithm>

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
		public:
			// Type-soecific interface...

		private:
			// Type-soecific privates...

			JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS;
			friend class OurProjectTypeRegistry;
		};
	}

	___________________________________
	/// "OurClassType.cpp":
	#include "OurClassType.h"
	namespace OurProjectNamespace {
		JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(OurClassType, 
			{
				// "RegisterType" logic...
			}, {
				// "UnregisterType" logic...
			});
	}

6. Once we've done these, "header/to/generate.impl.h" will automagically include "RegisterType" and "UnregisterType" calls in OurProjectTypeRegistry's constructor and destructor, respectively;
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
		TypeRegistrationClass(); \
		virtual void OnOutOfScope()const override; \
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

/// <summary> Defines RegisterType() and UnregisterType() static methods </<summary>>
#define JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS \
	static void RegisterType(); \
	static void UnregisterType()

/// <summary> Implements RegisteredClassType::RegisterType() and RegisteredClassType::UnregisterType() previously defined with JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS </summary>
#define JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(RegisteredClassType, RegisterCallbackBody, UnregisterCallbackBody) \
	void RegisteredClassType::RegisterType() { RegisterCallbackBody; } \
	void RegisteredClassType::UnregisterType() { UnregisterCallbackBody; }

namespace Jimara {
	class TypeInheritance;

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
		typedef TypeInheritance(*InheritanceInfoGetter)();
		InheritanceInfoGetter m_getInheritance;

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
		inline constexpr TypeId(const std::string_view& typeName, CheckTypeFn checkType, const std::type_info& typeInfo, const InheritanceInfoGetter& getInheritance)
			: m_typeName(typeName), m_checkType(checkType), m_typeInfo(&typeInfo), m_getInheritance(getInheritance) {}

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

		/// <summary> Parent type information (for a class type to have any, ParentTypes::Of<Type> has to be overloaded) </summary>
		inline const TypeInheritance InheritanceInfo()const;

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
	};

	/// <summary>
	/// Information about parent types of some class/struct
	/// Note: TypeId::InheritanceInfo relies on TypeInheritance::Of function and, therefore, it's up to the implementation to expose the parent types faithfully.
	/// </summary>
	class TypeInheritance {
		// Parent data
		Stacktor<TypeId, 1> m_parentTypes;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parents"> Parent types </param>
		/// <param name="count"> Parent type count </param>
		inline TypeInheritance(const TypeId* parents, size_t count = 1)
			: m_parentTypes((count > 0) ? parents : nullptr, (parents != nullptr) ? count : 0) {
			std::sort(m_parentTypes.Data(), m_parentTypes.Data() + m_parentTypes.Size());
			for (size_t i = 0; i < (m_parentTypes.Size() - 1); i++)
				while ((i < (m_parentTypes.Size() - 1)) && (m_parentTypes[i] == m_parentTypes[i + 1]))
					m_parentTypes.RemoveAt(i);
		}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="ParentCount"> Parent type count </typeparam>
		/// <param name="parents"> Parent types </param>
		template<size_t ParentCount>
		inline TypeInheritance(const TypeId(&parents)[ParentCount]) : TypeInheritance(parents, ParentCount) { }

	public:
		/// <summary> Default constructor </summary>
		inline TypeInheritance() {}

		/// <summary> Parent type count </summary>
		inline size_t ParentTypeCount()const { return m_parentTypes.Size(); }

		/// <summary>
		/// Parent type by index
		/// </summary>
		/// <param name="index"> Index of the parent type (valid range is [0 - ParentTypeCount()]) </param>
		/// <returns> Parent type id </returns>
		inline const TypeId& ParentType(size_t index)const { return m_parentTypes[index]; }

		/// <summary>
		/// Parent types of Type
		/// Note: Specialize this one for each type to 'register' information about inheritance (you can make that information junk, but I hope you won't)
		/// </summary>
		/// <typeparam name="Type"> Type, parents of which we care about </typeparam>
		/// <returns> Parent type collection </returns>
		template<typename Type>
		inline static TypeInheritance Of() { return TypeInheritance(); }
	};

	/// <summary> Default constructor </summary>
	inline constexpr TypeId::TypeId() : m_getInheritance([]() -> TypeInheritance { return TypeInheritance(); }) {}

	/// <summary> Parent type information (for a class type to have any, ParentTypes::Of<Type> has to be overloaded) </summary>
	inline const TypeInheritance TypeId::InheritanceInfo()const { return m_getInheritance(); }

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

		// Inheritance:
		constexpr const InheritanceInfoGetter getInheritance = []() -> TypeInheritance { return TypeInheritance::Of<Type>(); };

		// Create type id:
		return TypeId(TYPE_NAME, checkType, typeInfo, getInheritance);
	}

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
	/// <returns> Just TypeId::Of<Object>() </returns>
	template<> TypeInheritance TypeInheritance::Of<BuiltInTypeRegistrator>();
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
