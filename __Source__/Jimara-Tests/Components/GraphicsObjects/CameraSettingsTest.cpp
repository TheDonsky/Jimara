#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../TestEnvironment/TestEnvironment.h"
#include "Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "Components/GraphicsObjects/MeshRenderer.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/Camera.h"
#include "Data/Generators/MeshGenerator.h"


namespace Jimara {
	TEST(CameraSettingsTest, LayerFiltering) {
		Jimara::Test::TestEnvironment environment("Layer Filtering Test");

		environment.ExecuteOnUpdateNow([&]() {
			Reference<Transform> sun = Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(0.0f), Vector3(64.0f, 32.0f, 0.0f));
			Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
			Reference<Transform> back = Object::Instantiate<Transform>(environment.RootObject(), "Back");
			back->LookTowards(-sun->Forward());
			Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
			});

		Reference<MeshRenderer> rendererA;
		Reference<MeshRenderer> rendererB;
		Reference<MeshRenderer> rendererC;
		Reference<Camera> camera;

		// Create objects:
		environment.ExecuteOnUpdateNow([&]() {
			auto createRenderer = [&](auto createMesh, const Vector3& color, const Vector3& offset) {
				const Reference<TriMesh> mesh = createMesh();
				const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(
					environment.RootObject()->Context()->Graphics()->Device(), color);
				const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(
					Object::Instantiate<Transform>(environment.RootObject(), "rendererA", offset), "rendererA", mesh);
				renderer->SetMaterialInstance(material);
				return renderer;
			};
			rendererA = createRenderer(
				[]() { return GenerateMesh::Tri::Box(Vector3(-0.5f), Vector3(0.5f)); }, 
				Vector3(1.0f, 0.0f, 0.0f), Vector3(-1.0f, 0.0f, 0.0f));
			rendererB = createRenderer(
				[]() { return GenerateMesh::Tri::Cone(Vector3(0.0f, -0.5f, 0.0f), 1.0f, 0.5f, 16); },
				Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f));
			rendererC = createRenderer(
				[]() { return GenerateMesh::Tri::Cylinder(Vector3(0.0f), 0.5f, 1.0f, 16); },
				Vector3(0.0f, 0.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f));
			camera = environment.RootObject()->GetComponentInChildren<Camera>();
			});

		static const constexpr int sleepInterval = 1;
		static const constexpr GraphicsLayer layer0 = 0;
		static const constexpr GraphicsLayer layer1 = 1;
		static const constexpr GraphicsLayer layer2 = 2;
		static const constexpr GraphicsLayer layer3 = 3;

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Applying different layers...");
			environment.ExecuteOnUpdateNow([&]() {
				rendererA->SetLayer(layer0);
				rendererB->SetLayer(layer1);
				rendererC->SetLayer(layer2);
				});
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 0");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer0)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 1");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer1)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 2");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer2)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 0 and 1");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer0, layer1)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 0 and 2");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer0, layer2)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 1 and 2");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer1, layer2)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Rendering only Layer 3");
			environment.ExecuteOnUpdateNow([&]() { camera->RenderLayers(GraphicsLayerMask(layer3)); });
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Moving renderers to Layer 3");
			environment.ExecuteOnUpdateNow([&]() {
				rendererA->SetLayer(layer3);
				rendererB->SetLayer(layer3);
				rendererC->SetLayer(layer3);
				});
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Moving renderers to Layer 1 (and camera too)");
			environment.ExecuteOnUpdateNow([&]() {
				rendererA->SetLayer(layer1);
				rendererB->SetLayer(layer1);
				camera->RenderLayers(GraphicsLayerMask(layer1));
				rendererC->SetLayer(layer1);
				});
		}
	}

	TEST(CameraSettingsTest, ClearFlags) {
		Jimara::Test::TestEnvironment environment("Clear Flags Test");

		environment.ExecuteOnUpdateNow([&]() {
			Reference<Transform> sun = Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(0.0f), Vector3(64.0f, 32.0f, 0.0f));
			Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
			Reference<Transform> back = Object::Instantiate<Transform>(environment.RootObject(), "Back");
			back->LookTowards(-sun->Forward());
			Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
			});

		static const constexpr GraphicsLayer layerA = 0;
		static const constexpr GraphicsLayer layerB = 1;
		static const constexpr int sleepInterval = 3;

		Reference<MeshRenderer> rendererA;
		Reference<MeshRenderer> rendererB;
		Reference<Camera> cameraA;
		Reference<Camera> cameraB;

		// Create objects:
		environment.ExecuteOnUpdateNow([&]() {
			Transform* meshBaseTransform = Object::Instantiate<Transform>(environment.RootObject(), "meshBaseTransform");
			static void(*rotateGeometry)(Transform*) = [](Transform* transform) {
				transform->SetLocalEulerAngles(Math::Up() * (transform->LocalEulerAngles().y + transform->Context()->Time()->ScaledDeltaTime() * 180.0f));
			};
			meshBaseTransform->Context()->OnUpdate() += Callback<>(rotateGeometry, meshBaseTransform);

			auto createRenderer = [&](auto createMesh, const Vector3& color, const Vector3& offset) {
				const Reference<TriMesh> mesh = createMesh();
				const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(
					environment.RootObject()->Context()->Graphics()->Device(), color);
				const Reference<MeshRenderer> renderer = Object::Instantiate<MeshRenderer>(
					Object::Instantiate<Transform>(meshBaseTransform, "rendererA", offset), "rendererA", mesh);
				renderer->SetMaterialInstance(material);
				return renderer;
			};
			rendererA = createRenderer(
				[]() { return GenerateMesh::Tri::Cone(Vector3(0.0f, -0.5f, 0.0f), 1.0f, 0.5f, 16); },
				Vector3(0.0f, 1.0f, 0.0f), Vector3(-0.75f, 0.0f, 0.0f));
			rendererB = createRenderer(
				[]() { return GenerateMesh::Tri::Cylinder(Vector3(0.0f), 0.5f, 1.0f, 16); },
				Vector3(0.0f, 0.0f, 1.0f), Vector3(0.75f, 0.0f, 0.0f));
			

			cameraA = environment.RootObject()->GetComponentInChildren<Camera>();
			cameraB = Object::Instantiate<Camera>(cameraA, "CameraB");
			static void(*updateCameraB)(Camera * cameraB) = [](Camera* camera) {
				const Camera* parentCamera = camera->GetComponentInParents<Camera>(false);
				camera->SetFieldOfView(parentCamera->FieldOfView());
				camera->SetClearColor(Vector4(0.125f));
				camera->SetClosePlane(parentCamera->ClosePlane());
				camera->SetFarPlane(parentCamera->FarPlane());
			};
			cameraB->Context()->OnUpdate() += Callback<>(updateCameraB, cameraB.operator->());

			rendererA->SetLayer(layerA);
			cameraA->SetRendererPriority(1);
			cameraA->RenderLayers(GraphicsLayerMask(layerA));

			rendererB->SetLayer(layerB);
			cameraB->RenderLayers(GraphicsLayerMask(layerB));
			});


		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Do not clear color on B (Cylinder will appear on top of the cone)");
			environment.ExecuteOnUpdateNow([&]() { 
				cameraB->SetRendererFlags(Graphics::RenderPass::Flags::CLEAR_DEPTH | Graphics::RenderPass::Flags::RESOLVE_COLOR);
				});
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Do not clear depth on B (Only cyllinder will appera, but the cone will cut it out where it would have been)");
			environment.ExecuteOnUpdateNow([&]() {
				cameraB->SetRendererFlags(Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::RESOLVE_COLOR);
				});
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Do not clear color or depth (Will appear as if there's only a single camera)");
			environment.ExecuteOnUpdateNow([&]() {
				cameraB->SetRendererFlags(Graphics::RenderPass::Flags::RESOLVE_COLOR);
				});
		}

		{
			std::this_thread::sleep_for(std::chrono::seconds(sleepInterval));
			environment.SetWindowName("Do not resolve (Only triangle will be visible)");
			environment.ExecuteOnUpdateNow([&]() {
				cameraB->SetRendererFlags(Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH);
				});
		}
	}
}
