#include "SceneViewAxisVectors.h"
#include <Components/Lights/DirectionalLight.h>
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Generators/MeshConstants.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Environment/Rendering/LightingModels/ForwardRendering/ForwardLightingModel.h>

namespace Jimara {
	namespace Editor {
		struct SceneViewAxisVectors::Tools {
			class Viewport : public virtual ViewportDescriptor {
			private:
				const Reference<const ViewportDescriptor> m_gizmoSceneViewport;

			public:
				Matrix4 viewMatrix = Math::Identity();

				inline Viewport(SceneViewAxisVectors* owner)
					: ViewportDescriptor(owner->m_subscene->Context())
					, m_gizmoSceneViewport(owner->GizmoContext()->Viewport()->GizmoSceneViewport()) {}

				inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }

				inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
					return Math::Perspective(Math::Radians(32.0f), aspect, 0.1f, 8.0f);
				}

				inline virtual Vector4 ClearColor()const override { 
					return Vector4(0.0f, 0.0f, 0.0f, 0.0f); 
				}
			};
			
			static void UpdateSubscene(SceneViewAxisVectors* self) {
				self->m_subscene->Update(self->Context()->Time()->UnscaledDeltaTime());
				const Transform* sceneViewTransform = self->GizmoContext()->Viewport()->ViewportTransform();
				self->m_cameraTransform->SetWorldEulerAngles(sceneViewTransform->WorldEulerAngles());
				self->m_cameraTransform->SetWorldPosition(self->m_cameraTransform->Forward() * -4.0f);
				dynamic_cast<Viewport*>(self->m_viewport.operator->())->viewMatrix = Math::Inverse(self->m_cameraTransform->WorldMatrix());
			}

			static void ConstructSubscene(SceneViewAxisVectors* self) {
				// Create camera transform and lights:
				{
					self->m_cameraTransform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), "Camera Transform");
					Object::Instantiate<DirectionalLight>(self->m_cameraTransform, "Camera Transform Light");
				}

				// Create viewport and renderer:
				{
					self->m_viewport = Object::Instantiate<Viewport>(self);
					const Reference<RenderStack::Renderer> renderer = ForwardLightingModel::Instance()->CreateRenderer(
						self->m_viewport,
						GraphicsLayerMask::All(),
						Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH |
						Graphics::RenderPass::Flags::RESOLVE_COLOR);
					self->m_renderStack->AddRenderer(renderer);
				}

				// Create sphere in the center:
				{
					const Reference<Transform> transform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), "Central Sphere");
					transform->SetLocalScale(Vector3(0.25f));
					Object::Instantiate<MeshRenderer>(transform, "Central Sphere Renderer", MeshContants::Tri::Sphere());
				}

				// Create 'arrows':
				{
					auto createArrow = [&](const Vector3& direction) {
						std::stringstream name;
						name << "Axis " << direction;
						const Reference<Transform> transform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), name.str());
						transform->SetLocalScale(Vector3(0.25f, 0.5f, 0.25f));
						transform->SetWorldPosition(direction * 0.55f);
						name << " Renderer";
						const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(self->Context()->Graphics()->Device(), direction);
						Object::Instantiate<MeshRenderer>(transform, name.str(), MeshContants::Tri::Cone())->SetMaterialInstance(material);
						return transform;
					};
					createArrow(Math::Right())->SetWorldEulerAngles(Vector3(0.0f, 0.0f, -90.0f));
					createArrow(Math::Up());
					createArrow(Math::Forward())->SetWorldEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
				}

				// Create negative axis boxes:
				{
					auto createNegArrow = [&](const Vector3& direction) {
						std::stringstream name;
						name << "Axis[neg] " << direction;
						const Reference<Transform> transform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), name.str());
						transform->SetLocalScale(Vector3(0.175f, 0.3f, 0.175f));
						transform->SetWorldPosition(direction * -0.7f);
						name << " Renderer";
						const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(self->Context()->Graphics()->Device(), direction);
						Object::Instantiate<MeshRenderer>(transform, name.str(), MeshContants::Tri::Cube())->SetMaterialInstance(material);
						return transform;
					};
					createNegArrow(Math::Right())->SetWorldEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
					createNegArrow(Math::Up());
					createNegArrow(Math::Forward())->SetWorldEulerAngles(Vector3(-90.0f, 0.0f, 0.0f));
				}
			}

			static void DestructSubscene(SceneViewAxisVectors* self) {
				self->m_viewport = nullptr;
				self->m_cameraTransform = nullptr;
			}
		};

		SceneViewAxisVectors::SceneViewAxisVectors(Scene::LogicContext* context)
			: Component(context, "SceneViewAxisVectors")
			, GizmoGUI::Drawer(std::numeric_limits<float>::infinity())
			, m_subscene([&]() ->Reference<Scene> {
			Reference<GizmoScene::Context> gizmoContext = GizmoScene::GetContext(context);
			Scene::CreateArgs createArgs = {};
			{
				createArgs.logic.logger = context->Log();
				createArgs.logic.input = gizmoContext->EditorApplicationContext()->InputModule();
				createArgs.logic.assetDatabase = context->AssetDB();
			}
			{
				createArgs.graphics.graphicsDevice = context->Graphics()->Device();
				createArgs.graphics.shaderLoader = context->Graphics()->Configuration().ShaderLoader();
				EditorContext::SceneLightTypes lightTypes = gizmoContext->EditorApplicationContext()->LightTypes();
				createArgs.graphics.lightSettings.lightTypeIds = lightTypes.lightTypeIds;
				createArgs.graphics.lightSettings.perLightDataSize = lightTypes.perLightDataSize;
				createArgs.graphics.maxInFlightCommandBuffers = context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
				createArgs.graphics.synchPointThreadCount = 1;
				createArgs.graphics.renderThreadCount = 1;
			}
			{
				createArgs.physics.physicsInstance = context->Physics()->APIInstance();
				createArgs.physics.simulationThreadCount = 1;
			}
			{
				createArgs.audio.audioDevice = context->Audio()->AudioScene()->Device();
			}
			createArgs.createMode = Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS;
			return Scene::Create(createArgs);
				}())
			, m_renderStack([&]() -> Reference<RenderStack> {
					Reference<GizmoScene::Context> gizmoContext = GizmoScene::GetContext(context);
					return Object::Instantiate<RenderStack>(gizmoContext->TargetContext(), Size2(64, 64));
				}()) {
			if (m_subscene == nullptr)
				Context()->Log()->Error("SceneViewAxisVectors - Failed to create subscene for corner arrows!");
			else Tools::ConstructSubscene(this);
			Context()->Graphics()->OnGraphicsSynch() += Callback(&Tools::UpdateSubscene, this);
		}

		SceneViewAxisVectors::~SceneViewAxisVectors() {
			Context()->Graphics()->OnGraphicsSynch() -= Callback(&Tools::UpdateSubscene, this);
			Tools::DestructSubscene(this);
		}

		void SceneViewAxisVectors::OnDrawGizmoGUI() {
			{
				Reference<RenderImages> images = m_renderStack->Images();
				Graphics::TextureView* view = images->GetImage(RenderImages::MainColor())->Resolve();
				if (view != m_guiView) {
					m_guiView = view;
					if (m_guiView != nullptr) {
						Reference<Graphics::TextureSampler> sampler = m_guiView->CreateSampler();
						m_guiTexture = ImGuiRenderer::Texture(sampler);
					}
					else m_guiTexture = nullptr;
				}
			}
			if (m_guiTexture != nullptr) {
				const ImVec2 initialPosition = ImGui::GetCursorPos();
				const ImVec2 imageSize = [&]() {
					const Size3 size = m_guiView->TargetTexture()->Size();
					return ImVec2(static_cast<float>(size.x), static_cast<float>(size.y));
				}();
				const ImVec2 drawPosition = ImVec2(ImGui::GetWindowContentRegionMax().x - imageSize.x, ImGui::GetWindowContentRegionMin().y);
				ImGui::SetCursorPos(drawPosition);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::ImageButton(m_guiTexture->operator ImTextureID(), imageSize);
				ImGui::PopStyleColor(3);
				ImGui::SetCursorPos(initialPosition);
			}
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection SceneViewAxisVectors_GizmoConnection = Gizmo::ComponentConnection::Targetless<SceneViewAxisVectors>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::SceneViewAxisVectors>() {
		Editor::Gizmo::AddConnection(Editor::SceneViewAxisVectors_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneViewAxisVectors>() {
		Editor::Gizmo::RemoveConnection(Editor::SceneViewAxisVectors_GizmoConnection);
	}
}