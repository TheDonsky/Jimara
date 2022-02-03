#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include "Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ObjectIdRenderer.h"
#include "Data/Generators/MeshGenerator.h"
#include "Components/Camera.h"
#include "Components/GraphicsObjects/MeshRenderer.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			class IdRenderer : public virtual Scene::GraphicsContext::Renderer {
			private:
				const Reference<ObjectIdRenderer> m_renderer;

			public:
				inline IdRenderer(const LightingModel::ViewportDescriptor* viewport) 
					: m_renderer(ObjectIdRenderer::GetFor(viewport)) {}

				virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, Graphics::TextureView* targetTexture) final override {
					m_renderer->SetResolution(targetTexture->TargetTexture()->Size());
					ObjectIdRenderer::ResultBuffers result = m_renderer->GetLastResults();
					if (result.vertexNormalColor != nullptr)
						targetTexture->TargetTexture()->Blit(commandBufferInfo.commandBuffer, result.vertexNormalColor->TargetTexture());
				}

				inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override { report(m_renderer); }
			};

			class IdModel : public virtual LightingModel {
			public:
				inline static IdModel* Instance() {
					static IdModel instance;
					return &instance;
				}

				virtual inline Reference<Scene::GraphicsContext::Renderer> CreateRenderer(const ViewportDescriptor* viewport) final override {
					return Object::Instantiate<IdRenderer>(viewport);
				}
			};
		}

		// Renders normal color from ObjectIdRenderer
		TEST(ObjectIdRendererTest, NormalColor) {
			Jimara::Test::TestEnvironment environment("ObjectIdRendererTest - Normal Color");

			Reference<Camera> camera = environment.RootObject()->GetComponentInChildren<Camera>();
			ASSERT_NE(camera, nullptr);

			camera->SetSceneLightingModel(IdModel::Instance());

			environment.ExecuteOnUpdateNow([&]() {
				Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
				Reference<TriMesh> sphere = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 32, 16);
				Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", sphere);
				});
		}
	}
}
