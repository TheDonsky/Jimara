#include "AudioSourceGizmo.h"
#include <Components/Audio/AudioSource.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Generators/MeshFromSpline.h>
#include <Data/Generators/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		struct AudioSourceGizmo::Helpers {
			inline static void Initialize(AudioSourceGizmo* gizmo, TriMesh* mesh) {
				gizmo->m_transform = Object::Instantiate<Transform>(gizmo, "AudioSourceGizmo_Transform");
				Object::Instantiate<MeshRenderer>(gizmo->m_transform, "AudioSourceGizmo_Renderer", mesh)
					->SetLayer(static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY));
				gizmo->m_transform->SetEnabled(false);
			}

			inline static constexpr Gizmo::Filter FilterFlags() {
				return 
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED;
			}
		};

		AudioSourceGizmo::AudioSourceGizmo() {}
		AudioSourceGizmo::~AudioSourceGizmo() {}

		class AudioSourceGizmo::Source2D : public virtual AudioSourceGizmo {
		protected:
			inline virtual void Update()override {
				AudioSourceGizmo::Update();
				if (m_transform->Enabled())
					m_transform->SetWorldEulerAngles(GizmoContext()->Viewport()->ViewportTransform()->WorldEulerAngles());
			}

		public:
			inline static TriMesh* Shape() {
				static const Reference<TriMesh> SHAPE = [&]() -> Reference<TriMesh> {
					const MeshFromSpline::SplineVertex splineVerts[] = {
						MeshFromSpline::SplineVertex { Vector3(-0.125f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.125f, 0.0f) },
						MeshFromSpline::SplineVertex { Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.125f, 0.0f) },
						MeshFromSpline::SplineVertex { Vector3(0.125f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, -1.0f), Vector3(0.0f, 0.25f, 0.0f) }
					};
					auto splineFn = [&](uint32_t index) { return splineVerts[index]; };
					MeshFromSpline::SplineCurve spline = MeshFromSpline::SplineCurve::FromCall(&splineFn);
					const constexpr uint32_t ringCount = sizeof(splineVerts) / sizeof(MeshFromSpline::SplineVertex);

					const Vector2 ringVerts[] = { Vector2(0.0f, -1.0f), Vector2(0.0f, 1.0f) };
					auto ringFn = [&](uint32_t index) { return ringVerts[index]; };
					MeshFromSpline::RingCurve ring = MeshFromSpline::RingCurve::FromCall(&ringFn);
					const constexpr uint32_t ringSegments = sizeof(ringVerts) / sizeof(Vector2);

					return MeshFromSpline::Tri(spline, ringCount, ring, ringSegments, MeshFromSpline::Flags::NONE, "AudioSource2D");
				}();
				return SHAPE;
			}

			inline Source2D(Scene::LogicContext* context) 
				: Component(context, "AudioSourceGizmo2D") {
				Helpers::Initialize(this, Shape());
			}

			inline static const constexpr Gizmo::ComponentConnection Connection() {
				return Gizmo::ComponentConnection::Make<Source2D, AudioSource2D>(Helpers::FilterFlags());
			}
		};

		class AudioSourceGizmo::Source3D : public virtual AudioSourceGizmo {
		protected:
			inline virtual void Update()override {
				AudioSourceGizmo::Update();
				if (m_transform->Enabled())
					m_transform->SetWorldEulerAngles(TargetComponent()->GetTransfrom()->WorldEulerAngles());
			}

		public:
			inline static TriMesh* Shape() {
				static const Reference<TriMesh> SHAPE = [&]() -> Reference<TriMesh> {
					auto splineVertex = [](float z, float radius) {
						MeshFromSpline::SplineVertex vertex = {};
						vertex.position = Vector3(0.0f, 0.0f, z);
						vertex.right = Vector3(radius, 0.0f, 0.0f);
						vertex.up = Vector3(0.0f, radius, 0.0f);
						return vertex;
					};
					const MeshFromSpline::SplineVertex splineVerts[] = {
						splineVertex(-0.1f, 0.1f),
						splineVertex(0.0f, 0.1f),
						splineVertex(0.01f, 0.1f),
						splineVertex(0.01f, 0.09f),
						splineVertex(0.02f, 0.09f),
						splineVertex(0.09f, 0.15f),
						splineVertex(0.1f, 0.15f),
						splineVertex(0.1f, 0.14f),
						splineVertex(0.07f, 0.1f)
					};
					auto splineFn = [&](uint32_t index) { return splineVerts[index]; };
					MeshFromSpline::SplineCurve spline = MeshFromSpline::SplineCurve::FromCall(&splineFn);
					const constexpr uint32_t ringCount = sizeof(splineVerts) / sizeof(MeshFromSpline::SplineVertex);

					const constexpr uint32_t ringSegments = 24;
					const constexpr float angleStep = Math::Radians(360.0f) / static_cast<float>(ringSegments);
					auto ringFn = [&](uint32_t index) {
						const float angle = angleStep * index;
						return Vector2(std::cos(angle), std::sin(angle));
					};
					MeshFromSpline::RingCurve ring = MeshFromSpline::RingCurve::FromCall(&ringFn);

					return MeshFromSpline::Tri(spline, ringCount, ring, ringSegments, MeshFromSpline::Flags::CAP_ENDS, "AudioSource3D");
				}();
				return SHAPE;
			}

			inline Source3D(Scene::LogicContext* context) 
				: Component(context, "AudioSourceGizmo3D") {
				Helpers::Initialize(this, Shape());
			}
			
			inline static const constexpr Gizmo::ComponentConnection Connection() {
				return Gizmo::ComponentConnection::Make<Source3D, AudioSource3D>(Helpers::FilterFlags());
			}
		};

		void AudioSourceGizmo::Update() {
			Component* target = TargetComponent();
			if (target == nullptr) return;
			Transform* targetTransform = target->GetTransfrom();
			if (targetTransform != nullptr && target->ActiveInHeirarchy()) {
				m_transform->SetEnabled(true);
				m_transform->SetWorldPosition(targetTransform->WorldPosition());
			}
			else m_transform->SetEnabled(false);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::AudioSourceGizmo>() {
		Editor::Gizmo::AddConnection(Editor::AudioSourceGizmo::Source2D::Connection());
		Editor::Gizmo::AddConnection(Editor::AudioSourceGizmo::Source3D::Connection());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::AudioSourceGizmo>() {
		Editor::Gizmo::AddConnection(Editor::AudioSourceGizmo::Source2D::Connection());
		Editor::Gizmo::AddConnection(Editor::AudioSourceGizmo::Source3D::Connection());
	}
}
