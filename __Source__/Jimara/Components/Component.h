#pragma once
namespace Jimara {
	class Component;
	class ComponentSerializer;
	class Transform;
}
#include "../Core/Object.h"
#include "../Core/Systems/Event.h"
#include "../Environment/SceneContext.h"
#include "../Data/Serialization/ItemSerializers.h"
#include "../Core/Synch/SpinLock.h"
#include <vector>
#include <string>
#include <string_view>
#include <set>


namespace Jimara {
	/**
	Jimara's scene system is component-based, consisting of an arbitrary components in it's tree-like heirarchy.
	When you desire to add a custom behavoiur for the game, you, more than likely, will be adding a bounch of new component types, just like in most other engines.
	
	Naturally, you will want to expose some parameters from the component through the Editor for a level designer to comfortably use it and adjust some settings, 
	as well as to save them as a part of a serialized scene both during development and inside the published binaries;
	In order to do so, you are adviced to override GetSerializer() method and provide your own implementation of ComponentSerializer,
	exposing the fields though it according to the rules defined alongside the general definition of the ItemSerializer and it's standard child interfaces.

	Although that is enough for the system to be able to access the relevant internals, the Editor, as well as the Scene/Prefab serializers will need to identify 
	the serialized component types based solely on the stored scene files. In order to resolve this issue, we have ComponentSerializer::RegistryEntry class;
	You simply need to create a registry entry and assign a ComponentSerializer object to it and the system will understand that the type exists 
	(view ComponentSerializer::RegistryEntry::GetAll for further details).

	All of these is fine and dandy and as long as you take all these actions, the system will have no problem whatsoever fetching all the types and making the level designers' and 
	internal scene/asset serializers' job rather straightforward; however, one issue remains: If you do not reference all the registered types through the code, 
	depending on your build configuration and the compiler, some compilation units may get dropped, resulting in lost registry entries, even if they were defined as static constants.
	In order to fix that issue, we use JIMARA_REGISTER_TYPE(OurComponentType) alongside it's callback definitions and the appropriate pre-build step to guarantee the registration and 
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

			virtual Reference<const ComponentSerializer> GetSerializer()const override;

			// Component-specific methods...

		private:
			// Component-specific variables and private methods...

			// Type registration callbacks and friendship with type registrator 
			// (firend classes are generally not the best, but OurProjectTypeRegistry is automatically generated 
			// and will only access the registered callbacks implemented by us, so it's safe; it's here only to make callbacks private and prevent anyone else from invoking them)
			JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS;
			friend class OurProjectTypeRegistry;
		};
	}


	_______________________________________________________________________________________________________________________________________________________________
	/// "OurComponentType.cpp":

	namespace OurProjectNamespace {
		namespace {
			class OurComponentSerializer : public virtual ComponentSerializer {
			public:
				inline OurComponentSerializer() 
					: ItemSerializer("OurComponentType", "OurComponentType description")
					, ComponentSerializer("OurProjectNamespace/OurComponentType") {}

				inline Reference<Component> CreateComponent(Component* parent) const override {
					return Object::Instantiate<OurComponentType>(parent);
				}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
					OurComponentType* target = dynamic_cast<OurComponentType*>((Component*)targetAddr);
					target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

					// Expose the rest of the internals as defined alongside ItemSerializer...
				}

				inline static const ComponentSerializer* Instance() {
					static const OurComponentSerializer instance;
					return &instance;
				}
			};

			static ComponentSerializer::RegistryEntry OUR_COMPONENT_SERIALIZER_ENTRY;
		}

		OurComponentType::OurComponentType(Component* parent) : Component(parent, "DefaultComponentName") {
			// Component initialization....
		}

		Reference<const ComponentSerializer> OurComponentType::GetSerializer()const { return OurComponentSerializer::Instance(); }

		JIMARA_IMPLEMENT_TYPE_REGISTRATION_CALLBACKS(OurComponentType, 
			{ OUR_COMPONENT_SERIALIZER_ENTRY = OurComponentSerializer::Instance(); }, { OUR_COMPONENT_SERIALIZER_ENTRY = nullptr; });

		// Rest of the component-specific implementation...
	}
	*/



	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Component);

	/// <summary>
	/// A generic Component object that can exist as a part of a scene
	/// Note: Components are not thread-safe by design to avoid needlessly loosing performance, so be careful about how you manipulate them
	/// </summary>
	class Component : public virtual Object {
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

		/// <summary> Invoked, whenever the parent of the object gets changed (but not when the object is destroyed) </summary>
		Event<const Component*>& OnParentChanged()const;

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transfrom; can be nullptr) </summary>
		Transform* GetTransfrom();

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transfrom; can be nullptr) </summary>
		const Transform* GetTransfrom()const;

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
		/// Finds component of some type in parent heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will return itself </param>
		/// <returns> First Component of type in parent heirarchy, starting from self/parent if found; nullptr otherwise </returns>
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
		/// Finds component of some type in parent heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will return itself </param>
		/// <returns> First Component of type in parent heirarchy, starting from self/parent if found; nullptr otherwise </returns>
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
		/// Finds components of some type in parent heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the components to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will include itself too </param>
		/// <returns> All components in parent heirarchy that are of the correct type (empty, if none found) </returns>
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
		/// Finds components of some type in parent heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the components to search for </typeparam>
		/// <param name="includeSelf"> If true and the the component finds 'this' to be of a viable type, the component will include itself too </param>
		/// <returns> All components in parent heirarchy that are of the correct type (empty, if none found) </returns>
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
		/// Finds a component of some type in child heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="recursive"> If true, component will be searched for recursively </param>
		/// <returns> Child Component of a correct type if found; nullptr otherwise </returns>
		template<typename ComponentType>
		ComponentType* GetComponentInChildren(bool recursive = true)const {
			for (size_t i = 0; i < m_children.size(); i++) {
				ComponentType* component = dynamic_cast<ComponentType*>(m_children[i].operator->());
				if (component != nullptr) return component;
			}
			if (recursive) for (size_t i = 0; i < m_children.size(); i++) {
				ComponentType* component = m_children[i]->GetComponentInChildren<ComponentType>(true);
				if (component != nullptr) return component;
			}
			return nullptr;
		}

		/// <summary>
		/// Finds components of some type in child heirarchy
		/// </summary>
		/// <typeparam name="ComponentType"> Type of the component to search for </typeparam>
		/// <param name="found"> List of components to append findings to </param>
		/// <param name="recursive"> If true, the components will be searched for recursively </param>
		template<typename ComponentType>
		void GetComponentsInChildren(std::vector<ComponentType*>& found, bool recursive = true)const {
			for (size_t i = 0; i < m_children.size(); i++) {
				ComponentType* component = dynamic_cast<ComponentType*>(m_children[i].operator->());
				if (component != nullptr) found.push_back(component);
			}
			if (recursive) for (size_t i = 0; i < m_children.size(); i++)
				m_children[i]->GetComponentsInChildren<ComponentType>(found, true);
		}

		/// <summary>
		/// Finds components of some type in child heirarchy
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



	private:
		// Scene context
		const Reference<SceneContext> m_context;

		// Component name
		std::string m_name;

		// Parent component (never nullptr)
		std::atomic<Component*> m_parent;

		// Child index within the parent
		std::atomic<size_t> m_childId = 0;

		// Child components
		std::vector<Reference<Component>> m_children;

		// Event, invoked when the parent gets altered
		mutable EventInstance<const Component*> m_onParentChanged;

		// Event, invoked when the component destruction is requested
		mutable EventInstance<Component*> m_onDestroyed;

		// Temporary buffer for storing component references for various operations
		mutable std::vector<Component*> m_referenceBuffer;

		// Notifies about parent change
		void NotifyParentChange()const;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Component>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<Component>(const Callback<const Object*>& report);


	/// <summary>
	/// Component serializer
	/// Note: Report an instance of a concrete implementation through TypeIdDetails::GateTypeAttributesOf<RegisteredComponentType> for it to be visible to the system.
	/// </summary>
	class ComponentSerializer : public virtual Serialization::SerializerList {
	public:
		/// <summary>
		/// Creates a new instance of a component with a compatible type
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <returns> New component instance </returns>
		virtual Reference<Component> CreateComponent(Component* parent)const = 0;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="component"> Component to serialize </param>
		virtual void SerializeComponent(const Callback<Serialization::SerializedObject>& recordElement, Component* component)const = 0;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <typeparam name="RecordCallback"> Anything, that can be called with a Serialization::SerializedObject as an argument </typeparam>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="component"> Component to serialize </param>
		template<typename RecordCallback>
		inline void Serialize(const RecordCallback& recordElement, Component* component)const {
			void(*serialize)(const RecordCallback*, Serialization::SerializedObject) = [](const RecordCallback* record, Serialization::SerializedObject field) { (*record)(field); };
			SerializeComponent(Callback<Serialization::SerializedObject>(serialize, &recordElement), component);
		}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// Note: targetAddr should be a reinterpret cast from a Component pointer.
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object address (has to point to a Component) </param>
		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const final override {
			SerializeComponent(recordElement, reinterpret_cast<Component*>(targetAddr));
		}

		/// <summary>
		/// Overrides CreateComponent and takes care of Component to ComponentType casting inside SerializeComponent() method
		/// </summary>
		template<typename ComponentType>
		class Of;
	};

	/// <summary>
	/// Overrides CreateComponent and takes care of Component to ComponentType casting inside SerializeComponent() method
	/// </summary>
	template<typename ComponentType>
	class ComponentSerializer::Of : public virtual ComponentSerializer {
	public:
		/// <summary>
		/// Creates a new instance of a component with a compatible type
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <returns> New component instance </returns>
		inline virtual Reference<Component> CreateComponent(Component* parent)const override {
			return Object::Instantiate<ComponentType>(parent);
		}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="component"> Component to serialize </param>
		virtual void SerializeTarget(const Callback<Serialization::SerializedObject>& recordElement, ComponentType* target)const = 0;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="component"> Component to serialize </param>
		virtual void SerializeComponent(const Callback<Serialization::SerializedObject>& recordElement, Component* component)const final override {
			ComponentType* target = dynamic_cast<ComponentType*>(component);
			if (target == nullptr) return;
			else SerializeTarget(recordElement, target);
		}
	};
}
