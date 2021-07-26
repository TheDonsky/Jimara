#pragma once
#include "../../Core/Object.h"

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
