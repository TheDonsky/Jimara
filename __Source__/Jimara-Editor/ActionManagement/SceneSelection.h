#pragma once
#include <Environment/Scene/Scene.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Selection manager for a scene
		/// </summary>
		class SceneSelection : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Scene context </param>
			SceneSelection(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneSelection();

			/// <summary>
			/// Adds component to selection
			/// <para/ > Note: if component is nullptr or does not have the root object in parent heirarchy, it can not be selected
			/// </summary>
			/// <param name="component"> Component to select </param>
			void Select(Component* component);

			/// <summary>
			/// Selects a number of components
			/// </summary>
			/// <typeparam name="...Rest"> Component pointers of any count </typeparam>
			/// <param name="first"> 'First' Component to add </param>
			/// <param name="second"> 'Second' Component to add </param>
			/// <param name="...rest"> 'Third', 'Fourth' and so on </param>
			template<typename... Rest>
			inline void Select(Component* first, Component* second, Rest... rest) {
				std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
				Select(first);
				Select(second, rest...);
			}

			/// <summary>
			/// Selects a list of components
			/// </summary>
			/// <typeparam name="ReferenceType"> Anything that can be trivially type-casted to Component pointer </typeparam>
			/// <param name="components"> List of component </param>
			/// <param name="count"> Number of Components in the list </param>
			template<typename ReferenceType>
			inline void Select(const ReferenceType* components, size_t count) {
				if (components == nullptr) return;
				std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
				for (size_t i = 0; i < count; i++) 
					Select(components[i]);
			}

			/// <summary>
			/// Selects entire subtree under given root component
			/// </summary>
			/// <param name="root"> Subtree root component </param>
			void SelectSubtree(Component* root);

			/// <summary> Selects all components </summary>
			void SelectAll();

			/// <summary>
			/// Deselects component
			/// </summary>
			/// <param name="component"> Component to deselect </param>
			void Deselect(Component* component);

			/// <summary>
			/// Deselects a number of components
			/// </summary>
			/// <typeparam name="...Rest"> Component pointers of any count </typeparam>
			/// <param name="first"> 'First' Component to remove </param>
			/// <param name="second"> 'Second' Component to remove </param>
			/// <param name="...rest"> 'Third', 'Fourth' and so on </param>
			template<typename... Rest>
			inline void Deselect(Component* first, Component* second, Rest... rest) {
				std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
				Deselect(first);
				Deselect(second, rest...);
			}

			/// <summary>
			/// Deselects a list of components
			/// </summary>
			/// <typeparam name="ReferenceType"> Anything that can be trivially type-casted to Component pointer </typeparam>
			/// <param name="components"> List of component </param>
			/// <param name="count"> Number of Components in the list </param>
			template<typename ReferenceType>
			inline void Deselect(const ReferenceType* components, size_t count) {
				if (components == nullptr) return;
				std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
				for (size_t i = 0; i < count; i++) 
					Deselect(components[i]);
			}

			/// <summary>
			/// Deselects entire subtree under given root component
			/// </summary>
			/// <param name="root"> Subtree root component </param>
			void DeselectSubtree(Component* root);

			/// <summary> Deselects all currently selected Components </summary>
			void DeselectAll();

			/// <summary>
			/// Iterates over current selection
			/// <para/ > Note: Selection can not be altered while iterationg
			/// </summary>
			/// <typeparam name="ReportCallback"> Any callable object (function/callback) that can be called with Component pointer as an argument </typeparam>
			/// <param name="report"> Any callable object (function/callback) that can be called with Component pointer as an argument </param>
			template<typename ReportCallback>
			inline void Iterate(const ReportCallback& report)const {
				std::unique_lock<std::recursive_mutex> lock(m_context->UpdateLock());
				for (const Reference<Component> elem : m_selection)
					report(elem.operator->());
			}

			/// <summary>
			/// List of all currently selected Components
			/// <para/ > Note: This function creates the vector, so use with caution if you have performance considerations
			/// </summary>
			std::vector<Reference<Component>> Current()const;

			/// <summary> Invoked, when a new Component gets added to the selection </summary>
			Event<Component*>& OnComponentSelected()const;

			/// <summary>
			/// Invoked when any Component gets removed from the selection
			/// <para/ > Note: This also will get invoked if the Component got destroyed and was removed from the selection because of that
			/// </summary>
			Event<Component*>& OnComponentDeselected()const;

		private:
			// Scene context
			const Reference<Scene::LogicContext> m_context;

			// Set of currently selected components
			std::unordered_set<Reference<Component>> m_selection;

			// Invoked, when a new Component gets added to the selection
			mutable EventInstance<Component*> m_onComponentSelected;

			// Invoked when any Component gets removed from the selection
			mutable EventInstance<Component*> m_onComponentDeselected;

			// Component destruction listener
			void OnComponentDestroyed(Component* component);
		};
	}
}
