#pragma once
namespace Jimara {
	class Component; 
	class Transform;
}
#include "../Core/Object.h"
#include "../Core/Event.h"
#include "../Environment/SceneContext.h"
#include <vector>
#include <string>
#include <set>


namespace Jimara {
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
		Component(SceneContext* context, const std::string& name);

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component [Can not be nullptr] </param>
		Component(Component* parent, const std::string& name);

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

		/// <summary> Invoked, whenever the parent of the object gets changed (but not when the object is destroyed) </summary>
		Event<const Component*, Component*>& OnParentChanged()const;

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transfrom; can be nullptr) </summary>
		Jimara::Transform* Transfrom();

		/// <summary> Transform component (either self or on the closest parent that is or inherits Transfrom; can be nullptr) </summary>
		const Jimara::Transform* Transfrom()const;

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
		Event<const Component*>& OnDestroyed()const;

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
			for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
				ComponentType* component = dynamic_cast<ComponentType*>((*it).operator->());
				if (component != nullptr) return component;
			}
			if (recursive) for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
				ComponentType* component = (*it)->GetComponentInChildren<ComponentType>(true);
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
			for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
				ComponentType* component = dynamic_cast<ComponentType*>((*it).operator->());
				if (component != nullptr) found.push_back(component);
			}
			if (recursive) for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
				(*it)->GetComponentsInChildren<ComponentType>(found, true);
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

		// Child components
		std::set<Reference<Component>> m_children;

		// Event, invoked when the parent gets altered
		mutable EventInstance<const Component*, Component*> m_onParentChanged;

		// Event, invoked when the component destruction is requested
		mutable EventInstance<const Component*> m_onDestroyed;
	};
}
