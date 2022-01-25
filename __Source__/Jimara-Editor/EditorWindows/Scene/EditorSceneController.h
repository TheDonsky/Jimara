#pragma once
#include "../../Environment/JimaraEditor.h"
#include <Data/Generators/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		class EditorSceneController : public virtual Object {
		public:
			inline EditorSceneController(EditorContext* context) : m_context(context) {}

			inline EditorContext* Context()const { return m_context; }

			inline Reference<EditorScene> Scene()const {
				if (m_scene != nullptr) return m_scene;
				else return m_context->GetScene();
			}

			inline void SetScene(EditorScene* scene) {
				m_scene = scene;
			}

			inline Reference<EditorScene> GetOrCreateScene(bool createGlobalIfNotFound = true) {
				Reference<EditorScene> scene = Scene();
				if (scene == nullptr) {
					scene = Object::Instantiate<EditorScene>(Context());
					{
						Object::Instantiate<Component>(scene->RootObject(), "A");
						Object::Instantiate<Component>(Object::Instantiate<Component>(scene->RootObject(), "B"), "C");
					}
					if (createGlobalIfNotFound) Context()->SetScene(scene);
					else SetScene(scene);
				}
				return scene;
			}

		private:
			const Reference<EditorContext> m_context;
			Reference<EditorScene> m_scene;
		};
	}

	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::EditorSceneController>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}

