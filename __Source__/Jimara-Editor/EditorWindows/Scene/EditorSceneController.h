#pragma once
#include "../../Environment/JimaraEditor.h"
#include <Data/Generators/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Arbitary object that is tied to an editor scene 
		/// </summary>
		class EditorSceneController : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			inline EditorSceneController(EditorContext* context) : m_context(context) {}

			/// <summary> Editor context </summary>
			inline EditorContext* Context()const { return m_context; }

			/// <summary> Target [Editor]Scene (m_context->GetScene() by default) </summary>
			inline Reference<EditorScene> Scene()const {
				if (m_scene != nullptr) return m_scene;
				else return m_context->GetScene();
			}

			/// <summary>
			/// Sets target scene
			/// </summary>
			/// <param name="scene"> Target scene </param>
			inline void SetScene(EditorScene* scene) {
				m_scene = scene;
			}

			/// <summary>
			/// Requests a non-nullable EditorScene reference
			/// </summary>
			/// <param name="createGlobalIfNotFound"> If true, global EditorScene will be created if existing one is not found </param>
			/// <returns> Editor scene </returns>
			inline Reference<EditorScene> GetOrCreateScene(bool createGlobalIfNotFound = true) {
				Reference<EditorScene> scene = Scene();
				if (scene == nullptr) {
					scene = Object::Instantiate<EditorScene>(Context());
					if (createGlobalIfNotFound) Context()->SetScene(scene);
					else SetScene(scene);
				}
				return scene;
			}

		private:
			// Editor context
			const Reference<EditorContext> m_context;

			// Target scene (nullptr means main scene)
			Reference<EditorScene> m_scene;
		};
	}

	// TypeIdDetails for EditorSceneController
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorSceneController>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}

