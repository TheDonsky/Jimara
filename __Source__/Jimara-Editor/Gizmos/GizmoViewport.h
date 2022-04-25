#pragma once
#include <Environment/Scene/Scene.h>
#include <Environment/GraphicsContext/LightingModels/LightingModel.h>
#include <Components/Transform.h>

namespace Jimara {
	namespace Editor {
		class GizmoViewport : public virtual Object {
		public:
			GizmoViewport(Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext);

			virtual ~GizmoViewport();

			Transform* ViewportTransform();

			Size2 Resolution()const;

			void SetResolution(const Size2& resolution);

			inline float FieldOfView()const { return m_fieldOfView; }

			inline void SetFieldOfView(float fieldOfView) { m_fieldOfView.store(fieldOfView); }

			inline LightingModel::ViewportDescriptor* TargetSceneViewport()const { return m_targetViewport; }

			inline LightingModel::ViewportDescriptor* GizmoSceneViewport()const { return m_gizmoViewport; }

		private:
			const Reference<Scene::LogicContext> m_targetContext;
			const Reference<Scene::LogicContext> m_gizmoContext;
			const Reference<LightingModel::ViewportDescriptor> m_targetViewport;
			const Reference<LightingModel::ViewportDescriptor> m_gizmoViewport;

			Reference<Scene::GraphicsContext::Renderer> m_renderer;
			Reference<Component> m_rootComponent;
			Reference<Transform> m_transform;
			std::atomic<float> m_fieldOfView = 60.0f;
			Vector4 m_clearColor = Vector4(0.125f, 0.125f, 0.125f, 1.0f);

			void Update();
		};
	}
}
