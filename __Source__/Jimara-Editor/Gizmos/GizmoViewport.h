#pragma once
#include <Jimara/Environment/Scene/Scene.h>
#include <Jimara/Environment/Rendering/LightingModels/LightingModel.h>
#include <Jimara/Components/Transform.h>
#include <Jimara/Components/Camera.h>
#include "GizmoLayers.h"


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

			/// <summary> Projection mode for the view </summary>
			inline Camera::ProjectionMode ProjectionMode()const { return m_projectionMode; }

			/// <summary>
			/// Lets the user switch between perspective and orthographic projection modes
			/// </summary>
			/// <param name="mode"> Projection mode </param>
			inline void SetProjectionMode(Camera::ProjectionMode mode) { 
				m_projectionMode = (mode == Camera::ProjectionMode::PERSPECTIVE)
					? Camera::ProjectionMode::PERSPECTIVE : Camera::ProjectionMode::ORTHOGRAPHIC;
			}

			/// <summary> Viewport field of view </summary>
			inline float FieldOfView()const { return m_fieldOfView; }

			/// <summary>
			/// Sets viewport field of view
			/// </summary>
			/// <param name="fieldOfView"> Field of view to use </param>
			inline void SetFieldOfView(float fieldOfView) { m_fieldOfView.store(min(max(0.001f, fieldOfView), 179.9999f)); }

			/// <summary> Vertical size of the region, visible in orthographic mode </summary>
			inline float OrthographicSize()const { return m_orthographicSize; }
			
			/// <summary>
			/// Sets orthographic size
			/// </summary>
			/// <param name="value"> Size to use </param>
			inline void SetOrthographicSize(float size) { m_orthographicSize = size; }

			/// <summary> Graphics viewport with the target scene context </summary>
			inline ViewportDescriptor* TargetSceneViewport()const { return m_targetViewport; }

			/// <summary> Graphics viewport with the gizmo scene context </summary>
			inline ViewportDescriptor* GizmoSceneViewport()const { return m_gizmoViewport; }

			/// <summary> GizmoViewport's very own render stack </summary>
			inline RenderStack* ViewportRenderStack()const { return m_renderStack; }

			/// <summary>
			/// Short for HandleProperties::HandleSizeFor(this, location)
			/// </summary>
			/// <param name="location"> Location, to calculate size at </param>
			/// <returns> Gizmo size at give location </returns>
			float GizmoSizeAt(const Vector3& location)const;

		private:
			// target scene context
			const Reference<Scene::LogicContext> m_targetContext;

			// Gizmo scene context
			const Reference<Scene::LogicContext> m_gizmoContext;

			// Viewport render stack
			const Reference<RenderStack> m_renderStack;

			// Graphics viewport with the target scene context
			const Reference<ViewportDescriptor> m_targetViewport;

			// Graphics viewport with the gizmo scene context
			const Reference<ViewportDescriptor> m_gizmoViewport;

			// HandleProperties
			mutable Reference<Object> m_handleProperties;

			// Underlying renderer
			Reference<JobSystem::Job> m_renderer;

			// Root object of the viewport transform component
			mutable Reference<Component> m_rootComponent;

			// Viewport transform component
			mutable Reference<Transform> m_transform;

			// Perspective/Orthographics mode
			std::atomic<Camera::ProjectionMode> m_projectionMode = Camera::ProjectionMode::PERSPECTIVE;

			// Viewport field of view
			std::atomic<float> m_fieldOfView = 60.0f;

			// Orthographic size
			std::atomic<float> m_orthographicSize = 8.0f;

			// Target scene viewport clear color
			Vector4 m_clearColor = Vector4(0.125f, 0.125f, 0.125f, 1.0f);

			// Called on each graphics synch point
			void Update();
		};
	}
}
