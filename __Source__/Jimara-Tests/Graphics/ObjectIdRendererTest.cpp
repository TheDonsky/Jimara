#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include "Environment/Rendering/LightingModels/ObjectIdRenderer/ObjectIdRenderer.h"
#include "Environment/Rendering/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h"
#include "Data/Generators/MeshGenerator.h"
#include "Components/Camera.h"
#include "Components/Lights/PointLight.h"
#include "Components/GraphicsObjects/MeshRenderer.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			class IdRenderer : public virtual RenderStack::Renderer {
			private:
				const Reference<ObjectIdRenderer> m_renderer;

			public:
				inline IdRenderer(const ViewportDescriptor* viewport, const LayerMask& layers = LayerMask::All())
					: m_renderer(ObjectIdRenderer::GetFor(viewport, layers)) {}

				virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, RenderImages* images) final override {
					m_renderer->SetResolution(images->Resolution());
					ObjectIdRenderer::Reader reader(m_renderer);
					ObjectIdRenderer::ResultBuffers result = reader.LastResults();
					if (result.vertexNormalColor != nullptr)
						images->GetImage(RenderImages::MainColor())->Resolve()->
						TargetTexture()->Blit(commandBufferInfo.commandBuffer, result.vertexNormalColor->TargetView()->TargetTexture());
				}

				inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override { report(m_renderer); }
			};

			class IdModel : public virtual LightingModel {
			public:
				inline static IdModel* Instance() {
					static IdModel instance;
					return &instance;
				}

				virtual inline Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask, Graphics::RenderPass::Flags) final override {
					return Object::Instantiate<IdRenderer>(viewport);
				}
			};
		}

		// Renders normal color from ObjectIdRenderer
		TEST(ObjectIdRendererTest, NormalColor) {
			Jimara::Test::TestEnvironment environment("ObjectIdRendererTest - Normal Color");

			Reference<Camera> camera = environment.RootObject()->GetComponentInChildren<Camera>();
			ASSERT_NE(camera, nullptr);

			environment.ExecuteOnUpdateNow([&]() {
				camera->SetSceneLightingModel(IdModel::Instance());
				Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
				Reference<TriMesh> sphere = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 32, 16);
				Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", sphere);
				});
		}

		namespace {
			class QueryPosition : public virtual Scene::LogicContext::UpdatingComponent {
			private:
				const Reference<ViewportObjectQuery> m_query;
				const Reference<ObjectIdRenderer> m_renderer;

				inline static void OnQueryResult(Object* selfPtr, ViewportObjectQuery::Result result) {
					QueryPosition* self = dynamic_cast<QueryPosition*>(selfPtr);
					if (self == nullptr || self->Destroyed()) return;
					if (result.graphicsObject == nullptr) return;
					self->GetTransfrom()->SetWorldPosition(result.objectPosition + result.objectNormal * 0.125f);
					self->GetTransfrom()->LookTowards(result.objectNormal);
					if (self->Context()->Input()->KeyDown(OS::Input::KeyCode::MOUSE_FIRST))
						self->Context()->Log()->Info(result);
				}

			public:
				inline QueryPosition(Transform* transform, ViewportObjectQuery* query, ObjectIdRenderer* renderer)
					: Component(transform, "QueryPosition"), m_query(query), m_renderer(renderer) {}

			protected:

				inline virtual void Update() final override {
					m_query->QueryAsynch(
						Size2(Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X), Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y)),
						Callback(&QueryPosition::OnQueryResult), this);
					m_renderer->SetResolution(RenderStack::Main(Context())->Resolution());
				}
			};
		}

		// Renders normal color from ObjectIdRenderer
		TEST(ObjectIdRendererTest, ViewportObjectQuery_PositionAndNormal) {
			Jimara::Test::TestEnvironment environment("ObjectIdRendererTest - ViewportObjectQuery Position & Normal");

			Reference<Camera> camera = environment.RootObject()->GetComponentInChildren<Camera>();
			ASSERT_NE(camera, nullptr);

			Reference<ObjectIdRenderer> renderer = ObjectIdRenderer::GetFor(camera->ViewportDescriptor(), LayerMask(0));
			ASSERT_NE(renderer, nullptr);

			Reference<ViewportObjectQuery> query = ViewportObjectQuery::GetFor(camera->ViewportDescriptor(), LayerMask(0));
			ASSERT_NE(query, nullptr);

			environment.ExecuteOnUpdateNow([&]() {
				Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
				Reference<TriMesh> planeMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(8.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 8.0f), Size2(800, 800));
				{
					TriMesh::Writer writer(planeMesh);
					for (uint32_t i = 0; i < writer.VertCount(); i++) {
						MeshVertex& vertex = writer.Vert(i);
						auto getY = [&](float x, float z) { return cos(((x * x) + (z * z)) * 2.0f) * 0.05f; };
						vertex.position.y = getY(vertex.position.x, vertex.position.z);
						Vector3 dx = Vector3(vertex.position.x + 0.01f, 0.0f, vertex.position.z);
						dx.y = getY(dx.x, dx.z);
						Vector3 dz = Vector3(vertex.position.x, 0.0f, vertex.position.z + 0.01f);
						dz.y = getY(dz.x, dz.z);
						vertex.normal = Math::Normalize(Math::Cross(dz - vertex.position, dx - vertex.position));
					}
				}
				Object::Instantiate<MeshRenderer>(transform, "Surface", planeMesh);
				Reference<TriMesh> capsule = GenerateMesh::Tri::Capsule(Vector3(0.0f, 0.5f, 0.0f), 0.25f, 0.5f, 16, 8);
				Object::Instantiate<MeshRenderer>(transform, "Capsule", capsule);
				Object::Instantiate<QueryPosition>(Object::Instantiate<Transform>(environment.RootObject(), "LightTransform"), query, renderer);
				});

			environment.ExecuteOnUpdateNow([&]() {
				Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
				{
					Transform* lightTransform = Object::Instantiate<Transform>(transform, "Transform");
					lightTransform->SetLocalPosition(Vector3(0.0f, 0.0f, 1.0f));
					Object::Instantiate<PointLight>(lightTransform, "Light", Vector3(1.0f, 1.0f, 1.0f));
				}
				{
					Transform* meshTransform = Object::Instantiate<Transform>(transform, "Transform");
					meshTransform->SetLocalEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
					Reference<TriMesh> capsule = GenerateMesh::Tri::Capsule(Vector3(0.0f, 0.0f, 0.0f), 0.01f, 0.25f, 16, 8);
					Object::Instantiate<MeshRenderer>(meshTransform, "Normal", capsule)->SetLayer(1);
				}
				Object::Instantiate<QueryPosition>(transform, query, renderer);
				});
		}
	}
}
