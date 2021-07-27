#pragma once
#include "../../Core/Object.h"

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
	/** 
	Built-in type registrator, all internal Jimara objects rely on 
	(Instance only needed to make accessing various type registries possible; the runtime will work fine without this one) 
	*/
	JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(BuiltInTypeRegistrator);
}
