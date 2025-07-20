#pragma once
namespace Jimara {
	class Component;
	class Transform;
	class SceneContext;
}
#include "../Core/Systems/Event.h"
#include "../Data/Serialization/Serializable.h"
#include "../Core/Synch/SpinLock.h"
#include "../Core/BulkAllocated.h"
#include "../Core/TypeRegistration/ObjectFactory.h"
#include "../Data/Serialization/SerializedAction.h"
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <set>
#include <map>


namespace Jimara {
	/**
	Jimara's scene system is component-based, consisting of an arbitrary components in it's tree-like hierarchy.
	When you desire to add a custom behavoiur for the game, you, more than likely, will be adding a bunch of new component types, just like in most other engines.
	
	Naturally, you will want to expose some parameters from the component through the Editor for a level designer to comfortably use it and adjust some settings, 
	as well as to save them as a part of a serialized scene both during development and inside the published binaries;
	In order to do so, you are adviced to override Jimara::Serialization::Serializable::GetFields() method of your component type for displaying/storing custom settings 
	and expose ComponentFactory::Create<your component type> using TypeIdDetails::GetTypeAttributesOf for the editor to know about the fact that the component type exists.

	All of these is fine and dandy and as long as you take all these actions, the system will have no problem whatsoever fetching all the types and making the level designers' and 
	internal scene/asset serializers' job rather straightforward; however, one issue remains: The system will only be able to fetch your ComponentFactory from attributes
	if your component type is registered. You can invoke TypeId::Register() manually, but if you do not reference all the registered types through the code, 
	depending on your build configuration and the compiler, some compilation units may get dropped, resulting in lost registry entries, even if they were defined as static constants.
	In order to fix that issue, we use JIMARA_REGISTER_TYPE(OurComponentType) and the appropriate pre-build step to guarantee the registration and 
	manage it's lifecycle (view TypeRegistration and it's macros for additional insights).

	
	Here's an example, depicting an implementation of an arbitrary component type:
	
	_______________________________________________________________________________________________________________________________________________________________
	/// "OurComponentType.h":

	#include "OurProjectTypeRegistry.h" // Equivalent of Engine's BuiltInTypeRegistrator, but defined in your project using JIMARA_DEFINE_TYPE_REGISTRATION_CLASS(OurProjectTypeRegistry);

	namespace OurProjectNamespace {
		JIMARA_REGISTER_TYPE(OurProjectNamespace::OurComponentType);

		class OurComponentType : public virtual Jimara::Component {
		public:
			OurComponentType(Component* parent);

			// For exposing our fields
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

			// Rest of the component body...
		};
	}
	namespace Jimara {
		// Specialize parent class information for your type
		template<> inline void TypeIdDetails::GetParentTypesOf<OurProjectNamespace::OurComponentType>(const Callback<TypeId>& report) { 
			report(TypeId::Of<Jimara::Component>());
			// Rest of the types... This is not really crucial
		}

		// Specialize attribute set for your type
		template<> void TypeIdDetails::GetTypeAttributesOf<OurProjectNamespace::OurComponentType>(const Callback<const Object*>& report);
	}


	_______________________________________________________________________________________________________________________________________________________________
	/// "OurComponentType.cpp":

	namespace Jimara {
		template<> void TypeIdDetails::GetTypeAttributesOf<OurProjectNamespace::OurComponentType>(const Callback<const Object*>& report) { 
			static const auto factory = ComponentFactory::Create<OurProjectNamespace::OurComponentType>(
				"Name", "OurProjectNamespace/OurComponentType", "OurComponentType description");
			report(factory); 
		}
	}
	namespace OurProjectNamespace {
		OurComponentType::OurComponentType(Component* parent) : Component(parent, "DefaultComponentName") {
			// Component initialization....
		}

		void OurComponentType::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			// Expose parent fields:
			Component::GetFields(recordElement);

			// Expose the rest of the internals as defined alongside ItemSerializer...
		}

		// Rest of the component-specific implementation...
	}
	*/



	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Component);

#pragma warning(disable: 4275)
	/// <summary>
	/// A generic Component object that can exist as a part of a scene
	/// <para/> Notes: 
	/// <para/>		0. Components are not thread-safe by design to avoid needlessly loosing performance, so be careful about how you manipulate them;
	/// <para/>		1. Component implements WeaklyReferenceable interface; however, Weak references to Components will not be thread-safe and will only be usable in synch with the main update thread.
	/// </summary>
	class JIMARA_API Component
		: public virtual WeaklyReferenceable
		, public virtual Serialization::Serializable
		, public virtual Serialization::SerializedCallback::Provider {
	protected:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context [Can not be nullptr] </param>
		Component(SceneContext* context, const std::string_view& name);

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component [Can not be nullptr] </param>
		Component(Component* parent, const std::string_view& name = "Component");

		/// <summary> Virtual destructor </summary>
		virtual ~Component();

		/// <summary> Component name </summary>
		std::string& Name();

		/// <summary> Component name </summary>
		const std::string& Name()const;

		/// <summary> 
		/// True, if the component itself is enabled 
		/// Notes: 
		///		0. Enabled() means that the component is marked as 'Enabled'; ActiveInHierarchy() tells if the component and every link inside it's parent chain is active;
		///		1. State of the root object is ignored by the internal logic, so disabling it will not change anything.
		/// </summary>
		bool Enabled()const;

		/// <summary> 
		/// Sets the component enabled/disabled
		/// Notes: 
		///		0. Does not change the state of the parent chain and, 
		///		threfore, the component can be disabled in hierarchy even if it, itself is enabled;
		///		1. State of the root object is ignored by the internal logic, so disabling it will not change anything.
		/// </summary>
		/// <param name="enabled"> If true, the component will become enabled </param>
		void SetEnabled(bool enabled);

		/// <summary>
		/// True, if the component is active in hierarchy
		/// Notes: 
		///		0. Enabled() means that the component is marked as 'Enabled'; ActiveInHierarchy() tells if the component and every link inside it's parent chain is active;
		///		1. State of the root object is ignored by the internal logic, so disabling it will not change anything.
		/// </summary>
		bool ActiveInHierarchy()const;

		/// <summary> Scene context </summary>
		SceneContext* Context()const;

		/// <summary> Root object (highest level parent) </summary>
		Component* RootObject();

		/// <summary> Root object (highest level parent) </summary>
		const Component* RootObject()const;

		/// <summary> Parent component </summary>
		Component* Parent()const;

		/// <summary>
		/// Sets new parent component
		/// </summary>
		/// <param name="newParent"> New parent object to set (nullptr means the same as RootObject()) </param>
		virtual void SetParent(Component* newParent);

		/// <summary> Index of this component in it's parent hierarchy </summary>
		size_t IndexInParent()const;

		/// <summary>
		/// Moves self in the parent's child list
		/// </summary>
		/// <param name="index"> Desired child index in parent </param>
		void SetIndexInParent(size_t index);

		/// <summary> Short for SetParent(nullptr) or SetParent(RootObject()) </summary>
		void ClearParent();

		/// <summary> Number of child components </summary>
		size_t ChildCount()const;

		/// <summary>
		/// Child component by id
		/// </summary>
		/// <param name="index"> Child index (valid range is [0 - ChildCount())) </param>
		/// <returns> Child component </returns>
		Component* GetChild(size_t index)const;

		/// <summary>
		/// Sorts child components
		/// </summary>
		/// <param name="less"> 'Less' function </param>
		void SortChildren(const Function<bool, Component*, Component*>& less);

		/// <summary>
		/// Information about component parent change
		/// </summary>
		struct ParentChangeInfo {
			/// <summary> Component, parent of which has changed </summary>
			Component* component = nullptr;

			/// <summary> Old parent of the component </summary>
			Component* oldParent = nullptr;

			/// <summary> New parent of the component (same as component->Parent()) </summary>
			Component* newParent = nullptr;
		};

		/// <summary> Invoked, whenever the parent of the object gets changed (but not when the object is destroyed) </summary>
		Event<ParentChangeInfo>& OnParentChanged()const;

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transform; can be nullptr) </summary>
		Transform* GetTransform();

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transform; can be nullptr) </summary>
		const Transform* GetTransform()const;

		/// <summary> 
		/// Requests the destruction the component and all the child objects recursively;
		/// Notes: 
		///		0. This call triggers OnDestroyed() on all affected objects;
		///		1. Even if the code is meant to treat destroyed components as objects that no longer exist, 
		///		regular old reference counting still applies and, therefore, the user should be wary of the circular references
		///		and other memory-leak causing structures... Our framework is not magic and C++ has no inherent garbage collector :)
		/// </summary>
		void Destroy();

		/// <summary>
		/// Invoked, whenever the component gets destroyed.
		/// Notes:
		///		0. Being destroyed means that Destroy() was called and the scene no longer holds Component's reference or the scene itself went out of scope;
		///		1. Due to Reference's inherent reference counting, actual memory will be kept intact, unless the listeners do appropriate cleanup;
		///		2. Just because this one was invoked, it won't mean that the object got deleted and, therefore, BEWARE OF THE CIRCULAR REFERENCES IN YOUR CODE!!!!
		/// </summary>
		Event<Component*>& OnDestroyed()const;

		/// <summary>
		/// Becomes true after Destroy() call to it or any of it's parent
		/// Note: Normally, one should not hold a reference to a destroyed Component, but sometimes we may not have a better way to know...
		/// </summary>
		bool Destroyed()const;

		/// <summary>
		/// Finds component of some type in parent hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will return itself </param>
		/// <returns> First Component of type in parent hierarchy, starting from self/parent if found; nullptr otherwise </returns>
		template<typename ComponentType>
		ComponentType* GetComponentInParents(bool includeSelf = true) {
			Component* ptr = includeSelf ? this : (Component*)m_parent;
			while (ptr != nullptr) {
				ComponentType* component = dynamic_cast<ComponentType*>(ptr);
				if (component != nullptr) return component;
				else ptr = ptr->m_parent;
			}
			return nullptr;
		}

		/// <summary>
		/// Finds component of some type in parent hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will return itself </param>
		/// <returns> First Component of type in parent hierarchy, starting from self/parent if found; nullptr otherwise </returns>
		template<typename ComponentType>
		const ComponentType* GetComponentInParents(bool includeSelf = true)const {
			const Component* ptr = includeSelf ? this : (Component*)m_parent;
			while (ptr != nullptr) {
				const ComponentType* component = dynamic_cast<const ComponentType*>(ptr);
				if (component != nullptr) return component;
				else ptr = ptr->m_parent;
			}
			return nullptr;
		}

		/// <summary>
		/// Finds components of some type in parent hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the components to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will include itself too </param>
		/// <returns> All components in parent hierarchy that are of the correct type (empty, if none found) </returns>
		template<typename ComponentType>
		std::vector<ComponentType*> GetComponentsInParents(bool includeSelf = true) {
			std::vector<ComponentType*> found;
			Component* ptr = includeSelf ? this : (Component*)m_parent;
			while (ptr != nullptr) {
				ComponentType* component = dynamic_cast<ComponentType*>(ptr);
				if (component != nullptr) found.push_back(component);
				ptr = ptr->m_parent;
			}
			return found;
		}

		/// <summary>
		/// Finds components of some type in parent hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the components to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will include itself too </param>
		/// <returns> All components in parent hierarchy that are of the correct type (empty, if none found) </returns>
		template<typename ComponentType>
		std::vector<const ComponentType*> GetComponentsInParents(bool includeSelf = true)const {
			std::vector<const ComponentType*> found;
			const Component* ptr = includeSelf ? this : (Component*)m_parent;
			while (ptr != nullptr) {
				const ComponentType* component = dynamic_cast<const ComponentType*>(ptr);
				if (component != nullptr) found.push_back(component);
				ptr = ptr->m_parent;
			}
			return found;
		}

		/// <summary>
		/// Finds a component of some type in child hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="recursive"> If true, component will be searched for recursively </param>
		/// <returns> Child Component of a correct type if found; nullptr otherwise </returns>
		template<typename ComponentType>
		ComponentType* GetComponentInChildren(bool recursive = true)const {
			for (size_t i = 0; i < m_children.size(); i++) {
				ComponentType* component = dynamic_cast<ComponentType*>(m_children[i]);
				if (component != nullptr) return component;
			}
			if (recursive) for (size_t i = 0; i < m_children.size(); i++) {
				ComponentType* component = m_children[i]->GetComponentInChildren<ComponentType>(true);
				if (component != nullptr) return component;
			}
			return nullptr;
		}

		/// <summary>
		/// Finds components of some type in child hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <typeparam name="ReferenceType"> 
		///		Component*/const Component/Reference<Component>/Reference<const Component> or something that can be constructed from a component reference 
		/// </typeparam>
		/// <param name="found"> List of components to append findings to </param>
		/// <param name="recursive"> If true, the components will be searched for recursively </param>
		template<typename ComponentType, typename ReferenceType>
		void GetComponentsInChildren(std::vector<ReferenceType>& found, bool recursive = true)const {
			for (size_t i = 0; i < m_children.size(); i++) {
				Component* child = m_children[i];
				ComponentType* component = dynamic_cast<ComponentType*>(child);
				if (component != nullptr) found.push_back(component);
				if (recursive && (child != nullptr))
					child->GetComponentsInChildren<ComponentType>(found, recursive);
			}
		}

		/// <summary>
		/// Finds components of some type in child hierarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="recursive"> If true, the components will be searched for recursively </param>
		/// <returns> List of components of the type, found in children (naturally, empty if none found) </returns>
		template<typename ComponentType>
		std::vector<ComponentType*> GetComponentsInChildren(bool recursive = true)const {
			std::vector<ComponentType*> found;
			GetComponentsInChildren<ComponentType>(found, recursive);
			return found;
		}

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;


	protected:
		/// <summary> 
		/// Invoked by the scene on the first frame this component gets instantiated
		/// <para/> Note: Can be invoked several times per frame, including the main logic Update loop and synch points (but not immediately after creation)
		/// </summary>
		inline virtual void OnComponentInitialized() {}

		/// <summary>
		/// Invoked after the component gets enabled for the first time
		/// <para/> Notes: 
		///	<para/>		0. Can be invoked several times per frame, including the main logic Update loop and synch points (but not immediately after creation/first enabling);
		///	<para/>		1. Gets invoked when the component gets instantiated and becomes active in hierarchy;
		///	<para/>		2. Invoked after corresponding OnComponentEnabled() callback.
		/// </summary>
		inline virtual void OnComponentStart() {}

		/// <summary>
		/// Invoked, whenever the component becomes active in herarchy
		/// <para/> Note: Can be invoked several times per frame, including the main logic Update loop and synch points (but not immediately after enabling);
		/// </summary>
		inline virtual void OnComponentEnabled() {}

		/// <summary>
		/// Invoked, whenever the component stops being active in herarchy
		/// <para/> Notes: 
		///	<para/>		0. Can be invoked several times per frame, including the main logic Update loop and synch points (but not immediately after disabling);
		///	<para/>		1. Will 'automagically' be invoked before the OnComponentDestroyed() callback.
		/// </summary>
		inline virtual void OnComponentDisabled() {}

		/// <summary>
		/// Invoked, whenever the component parent chain gets dirty
		/// <para/> Notes: 
		///	<para/> 	0. Can be invoked several times per frame, including the main logic Update loop and synch points (but not immediately after parent change);
		///	<para/> 	1. Will be invoked even if the parent chain ultimately stays the same; 
		///				only requirenment is for SetParent() to be invoked at least one in parent hierarchy with a different parent.
		/// </summary>
		inline virtual void OnParentChainDirty() {}

		/// <summary>
		/// Invoked, when the component gets destroyed
		/// Note: Invoked before OnDestroyed() event gets fired
		/// </summary>
		inline virtual void OnComponentDestroyed() {}





	public:
		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Fills WeakReferenceHolder with a StrongReferenceProvider, 
		/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override;

		/// <summary>
		/// [Only intended to be used by WeakReference<>; not safe for general use] Should clear the link to the StrongReferenceProvider;
		/// <para/> Address of the holder has to be the same as the one, previously passed to FillWeakReferenceHolder() method
		/// <para/> Note that this is not thread-safe!
		/// </summary>
		/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
		virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override;





	private:
		// Scene context
		const Reference<SceneContext> m_context;

		// Component name
		std::string m_name;

		// Flags
		enum class Flags : uint8_t {
			ENABLED = 1 << 0,
			DESTROYED = 1 << 1,
			STARTED = 1 << 2
		};
		std::atomic<uint8_t> m_flags = static_cast<uint8_t>(Flags::ENABLED);

		// Parent component (never nullptr)
		std::atomic<Component*> m_parent;

		// Child index within the parent
		std::atomic<size_t> m_childId = 0;

		// Child components
		std::vector<Component*> m_children;

		// Event, invoked when the parent gets altered
		mutable EventInstance<ParentChangeInfo> m_onParentChanged;

		// Event, invoked when the component destruction is requested
		mutable EventInstance<Component*> m_onDestroyed;

		// Weak-Refrence restore:
		class StrongReferenceProvider;
		Reference<WeaklyReferenceable::StrongReferenceProvider> m_weakObj;

		// Scene context can invoke a few lifetime-related events
		friend class SceneContext;
	};
#pragma warning(default: 4275)


	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Component>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<Component>(const Callback<const Object*>& report);

	/// <summary>
	/// Component factory;
	/// <para/> Notes:
	/// <para/>		0. Report an instance of a concrete implementation through TypeIdDetails::GateTypeAttributesOf<ComponentType> for it to be visible to the system;
	/// <para/>		1. Argument passed to the factory will be the parent component.
	/// </summary>
	using ComponentFactory = ObjectFactory<Component, Component*>;
}
