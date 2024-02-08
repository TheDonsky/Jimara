#pragma once
#include <Jimara/Environment/Scene/Scene.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Object, capable of storing and retrieving components from/to the OS clipboard
		/// </summary>
		class SceneClipboard : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Target scene context (can not be null) </param>
			SceneClipboard(Scene::LogicContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~SceneClipboard();

			/// <summary>
			/// Copies component heirarchies to clipboard
			/// <para/> Notes: 
			///		<para/> 0. Call will automatically prevent duplication of the components that have parents from the same collection;
			///		<para/> 1. If the collection is empty, previous clipboard record will be cleared regardless.
			/// </summary>
			/// <param name="roots"> Roots of component heirarchies to copy </param>
			void CopyComponents(const std::vector<Reference<Component>>& roots);

			/// <summary>
			/// Copies component heirarchies to clipboard
			/// <para/> Notes: 
			///		<para/> 0. Call will automatically prevent duplication of the components that have parents from the same collection;
			///		<para/> 1. If the collection is empty, previous clipboard record will be cleared regardless.
			/// </summary>
			/// <typeparam name="ReferenceType"> Any type of a reference to a Component (could be raw pointer, Reference<> or anything in between) </typeparam>
			/// <param name="roots"></param>
			/// <param name="count"></param>
			template<typename ReferenceType>
			inline void CopyComponents(const ReferenceType* roots, size_t count) {
				static thread_local std::vector<Reference<Component>> elems;
				elems.clear();
				if (roots != nullptr)
					for (size_t i = 0; i < count; i++)
						elems.push_back(roots[i]);
				CopyComponents(elems);
				elems.clear();
			}

			/// <summary>
			/// Copies component heirarchy to clipboard
			/// <para/> Note: If the root is nullptr, previous clipboard record will be cleared regardless.
			/// </summary>
			/// <param name="root"> Root of a heirarchy to copy </param>
			inline void CopyComponents(Component* root) { CopyComponents(&root, 1); }

			/// <summary>
			/// Pastes previously copied component heirarchies from clipboard as children of a given component
			/// </summary>
			/// <param name="parent"> Component to be used as the parent of instantiated components (if nullptr is provided, call will do nothing) </param>
			/// <returns> List of the instantiated 'root-level' components (ei the ones created directly under the parent) </returns>
			std::vector<Reference<Component>> PasteComponents(Component* parent);

		private:
			// Target context
			const Reference<Scene::LogicContext> m_context;

			// Internal GUID-s
			std::unordered_map<Reference<Component>, GUID> m_objectToId;
			std::unordered_map<GUID, Reference<Component>> m_idToObject;

			// Helper functions
			struct Tools;
		};
	}
}
