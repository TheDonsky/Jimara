#pragma once
namespace Jimara {
	class Component; 
	class Transform;
}
#include "../Core/Object.h"
#include "../Core/Event.h"
#include "../Environment/SceneContext.h"
#include <vector>
#include <set>


namespace Jimara {
	class Component : public virtual Object {
	protected:
		Component(SceneContext* context);

		Component(Component* parent);

	public:
		virtual ~Component();

		SceneContext* Context()const;

		Component* RootObject();

		const Component* RootObject()const;

		Component* Parent()const;

		virtual void SetParent(Component* newParent);

		void ClearParent();

		Event<const Component*, Component*>& OnParentChanged()const;

		Jimara::Transform* Transfrom();

		const Jimara::Transform* Transfrom()const;

		void Destroy();

		Event<const Component*>& OnDestroyed()const;

		template<typename ComponentType>
		ComponentType* GetComponentInParents(bool includeSelf = true) {
			Component* ptr = includeSelf ? this : m_parent;
			while (ptr != nullptr) {
				ComponentType* component = dynamic_cast<ComponentType*>(m_parent);
				if (component != nullptr) return component;
				else ptr = ptr->m_parent;
			}
			return nullptr;
		}

		template<typename ComponentType>
		const ComponentType* GetComponentInParents(bool includeSelf = true)const {
			const Component* ptr = includeSelf ? this : m_parent;
			while (ptr != nullptr) {
				const ComponentType* component = dynamic_cast<const ComponentType*>(m_parent);
				if (component != nullptr) return component;
				else ptr = ptr->m_parent;
			}
			return nullptr;
		}

		template<typename ComponentType>
		std::vector<ComponentType*> GetComponentsInParents(bool includeSelf = true) {
			std::vector<ComponentType*> found;
			Component* ptr = includeSelf ? this : m_parent;
			while (ptr != nullptr) {
				ComponentType* component = dynamic_cast<ComponentType*>(m_parent);
				if (component != nullptr) found.push_back(component);
				ptr = ptr->m_parent;
			}
			return found;
		}

		template<typename ComponentType>
		std::vector<const ComponentType*> GetComponentsInParents(bool includeSelf = true)const {
			std::vector<const ComponentType*> found;
			const Component* ptr = includeSelf ? this : m_parent;
			while (ptr != nullptr) {
				const ComponentType* component = dynamic_cast<const ComponentType*>(m_parent);
				if (component != nullptr) found.push_back(component);
				ptr = ptr->m_parent;
			}
			return found;
		}

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

		template<typename ComponentType>
		void GetComponentsInChildren(std::vector<ComponentType*>& found, bool recursive = true)const {
			for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
				ComponentType* component = dynamic_cast<ComponentType*>((*it).operator->());
				if (component != nullptr) found.push_back(component);
			}
			if (recursive) for (std::set<Reference<Component>>::const_iterator it = m_children.begin(); it != m_children.end(); ++it)
				(*it)->GetComponentsInChildren<ComponentType>(found, true);
		}

		template<typename ComponentType>
		std::vector<ComponentType*> GetComponentsInChildren(bool recursive = true)const {
			std::vector<ComponentType*> found;
			GetComponentsInChildren<ComponentType>(found, recursive);
			return found;
		}


	private:
		const Reference<SceneContext> m_context;

		Component* m_parent;

		std::set<Reference<Component>> m_children;

		mutable EventInstance<const Component*, Component*> m_onParentChanged;

		mutable EventInstance<const Component*> m_onDestroyed;
	};
}
