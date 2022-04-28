#pragma once
#include <Environment/Scene/Scene.h>
#include <Environment/GraphicsContext/LightingModels/LightingModel.h>
#include <Components/Transform.h>

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Viewport/Renderer of the GizmoScene.
		/// <para/> Renders target scene and overlays GizmoScene content on top of it
		/// </summary>
		class GizmoViewport : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="targetContext"> Target context </param>
			/// <param name="gizmoContext"> Gizmo scene context </param>
			GizmoViewport(Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext);

			/// <summary> Virtual destructor </summary>
			virtual ~GizmoViewport();

			/// <summary> Gizmo scene viewport transform for SceneView navigation </summary>
			Transform* ViewportTransform()const;

			/// <summary> Resolution for the underlying renderer (automatically updated by SceneView for the main one) </summary>
			Size2 Resolution()const;

			/// <summary>
			/// Sets resolution for the underlying renderer (automatically called by SceneView for the main one)
			/// </summary>
			/// <param name="resolution"> Rendering resolution </param>
			void SetResolution(const Size2& resolution);

			/// <summary> Viewport field of view </summary>
			inline float FieldOfView()const { return m_fieldOfView; }

			/// <summary>
			/// Sets viewport field of view
			/// </summary>
			/// <param name="fieldOfView"> Field of view to use </param>
			inline void SetFieldOfView(float fieldOfView) { m_fieldOfView.store(min(max(0.001f, fieldOfView), 179.9999f)); }

			/// <summary> Graphics viewport with the target scene context </summary>
			inline LightingModel::ViewportDescriptor* TargetSceneViewport()const { return m_targetViewport; }

			/// <summary> Graphics viewport with the gizmo scene context </summary>
			inline LightingModel::ViewportDescriptor* GizmoSceneViewport()const { return m_gizmoViewport; }

			/// <summary> Base size scalar for gizmos (it's adviced, they all appear the same size, regardless of the viewport distance) </summary>
			inline float GizmoSize()const { return m_gizmoSize; }

			/// <summary>
			/// Sets base gizmo size
			/// </summary>
			/// <param name="size"> New size to use </param>
			inline void SetGizmoSize(float size) { m_gizmoSize = size; }

			/// <summary>
			/// Calculates scaled gizmo size, based on it's position, distance from viewport and global gizmo scale
			/// </summary>
			/// <param name="location"> Location, to calculate size at </param>
			/// <returns> Gizmo size at give location </returns>
			inline float GizmoSizeAt(const Vector3& location)const { 
				const Transform* viewportTransform = ViewportTransform();
				return m_gizmoSize * Math::Dot(location - viewportTransform->WorldPosition(), viewportTransform->Forward());
			}

		private:
			// target scene context
			const Reference<Scene::LogicContext> m_targetContext;

			// Gizmo scene context
			const Reference<Scene::LogicContext> m_gizmoContext;

			// Graphics viewport with the target scene context
			const Reference<LightingModel::ViewportDescriptor> m_targetViewport;

			// Graphics viewport with the gizmo scene context
			const Reference<LightingModel::ViewportDescriptor> m_gizmoViewport;

			// Base gizmo size
			float m_gizmoSize = 0.25f;

			// Underlying renderer
			Reference<JobSystem::Job> m_renderer;

			// Root object of the viewport transform component
			mutable Reference<Component> m_rootComponent;

			// Viewport transform component
			mutable Reference<Transform> m_transform;

			// Viewport field of view
			std::atomic<float> m_fieldOfView = 60.0f;

			// Target scene viewport clear color
			Vector4 m_clearColor = Vector4(0.125f, 0.125f, 0.125f, 1.0f);

			// Called on each graphics synch point
			void Update();
		};
	}
}
