#include "SceneViewAxisVectors.h"
#include <Jimara/Core/Stopwatch.h>
#include <Jimara/Components/Lights/DirectionalLight.h>
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/Data/Geometry/MeshConstants.h>
#include <Jimara/Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Jimara/Environment/Rendering/LightingModels/ForwardRendering/ForwardLightingModel_Opaque_Pass.h>
#include <Jimara/OS/Input/NoInput.h>

namespace Jimara {
	namespace Editor {
		struct SceneViewAxisVectors::Tools {
			class Viewport : public virtual ViewportDescriptor {
			public:
				const Reference<GizmoViewport> gizmoSceneViewport;
				Matrix4 viewMatrix = Math::Identity();

				inline static const constexpr float FieldOfView() { return 32.0f; }

				inline Viewport(SceneViewAxisVectors* owner)
					: ViewportDescriptor(owner->m_subscene->Context())
					, gizmoSceneViewport(owner->GizmoContext()->Viewport()) {}

				inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }

				inline virtual Matrix4 ProjectionMatrix()const override {
					return Math::Perspective(FieldOfView(), 1.0f, 0.1f, 8.0f);
				}

				inline virtual Vector4 ClearColor()const override { 
					return Vector4(0.0f, 0.0f, 0.0f, 0.0f); 
				}
			};

			class RotateToTarget : public virtual Object {
			private:
				const Reference<Viewport> m_viewport;
				const Vector3 m_targetAngles;
				Stopwatch m_timer;

				inline void Update(Object*) {
					const constexpr float ANIMATION_TIME = 0.5f;
					const float elapsed = m_timer.Elapsed();
					const float percentage = (elapsed / ANIMATION_TIME);
					Transform* viewportTransform = m_viewport->gizmoSceneViewport->ViewportTransform();
					if (percentage >= 1.0f) 
						viewportTransform->SetWorldEulerAngles(m_targetAngles);
					else {
						viewportTransform->SetWorldEulerAngles(Math::LerpAngles(viewportTransform->WorldEulerAngles(), m_targetAngles, percentage));
						m_viewport->Context()->ExecuteAfterUpdate(Callback(&RotateToTarget::Update, this), this);
					}
				}

			public:
				inline RotateToTarget(Viewport* viewport, Vector3 targetAngles) 
					: m_viewport(viewport), m_targetAngles(targetAngles) {
					viewport->Context()->ExecuteAfterUpdate(Callback(&RotateToTarget::Update, this), this);
				}
			};

			static const constexpr float CenterRadius() { return 0.25f; }
			static const constexpr float AxisHandleCenterOffset() { return 0.7f; }
			static const constexpr float AxisHandleHandleRadius() { return 0.35f; }
			
			static void UpdateSubscene(SceneViewAxisVectors* self) {
				// Update subscene:
				self->m_subscene->Update(self->Context()->Time()->UnscaledDeltaTime());
				
				// Update viewport transform:
				{
					const Transform* sceneViewTransform = self->GizmoContext()->Viewport()->ViewportTransform();
					self->m_cameraTransform->SetWorldEulerAngles(sceneViewTransform->WorldEulerAngles());
					self->m_cameraTransform->SetWorldPosition(self->m_cameraTransform->Forward() * -4.0f);
				}

				// Update viewport:
				dynamic_cast<Viewport*>(self->m_viewport.operator->())->viewMatrix = Math::Inverse(self->m_cameraTransform->WorldMatrix());

				// Enable/disable arrows:
				{
					const Vector3 viewportForward = self->m_cameraTransform->Forward();
					for (size_t i = 0; i < self->m_arrowTransforms.size(); i++) {
						Transform* arrowTransform = self->m_arrowTransforms[i];
						const Vector3 direction = Math::Normalize(arrowTransform->LocalPosition());
						arrowTransform->SetEnabled(std::abs(Math::Dot(direction, viewportForward)) < 0.999f);
					}
				}
			}

			static void OnArrowClicked(const Transform* transform, Viewport* viewport) {
				const Vector3 position = transform->LocalPosition();
				const Vector3 direction = Math::Normalize(-position);
				Vector3 angles = (std::abs(Math::Dot(direction, Math::Up())) > 0.001f)
					? Vector3(direction.y >= 0.0f ? -90.0f : 90.0f, 0.0f, 0.0f)
					: Math::EulerAnglesFromMatrix(Math::LookTowards(direction));
				angles.z = 0.0f;
				Object::Instantiate<RotateToTarget>(viewport, angles);
			}

			static void CenterClicked(SceneViewAxisVectors* self) {
				self->GizmoContext()->Viewport()->SetProjectionMode(
					self->GizmoContext()->Viewport()->ProjectionMode() == Camera::ProjectionMode::PERSPECTIVE
					? Camera::ProjectionMode::ORTHOGRAPHIC : Camera::ProjectionMode::PERSPECTIVE);
			}

			static void OnClick(SceneViewAxisVectors* self, Size2 clickedPos, Size2 imageSize) {
				const Vector2 offset = Vector2(
					(static_cast<float>(clickedPos.x) / static_cast<float>(imageSize.x) * 2.0f) - 1.0f,
					(static_cast<float>(clickedPos.y) / static_cast<float>(imageSize.y) * -2.0f) + 1.0f);
				const float aspect = static_cast<float>(imageSize.x) / static_cast<float>(imageSize.y);
				const float tangent = std::tan(Math::Radians(Viewport::FieldOfView()) * 0.5f);
				
				const Vector3 origin = self->m_cameraTransform->WorldPosition();
				const Vector3 cameraForward = self->m_cameraTransform->Forward();
				const Vector3 rawOffsetDirection =
					(self->m_cameraTransform->Right() * (offset.x * aspect * tangent)) +
					(self->m_cameraTransform->Up() * (offset.y * tangent));
				const Vector3 direction = Math::Normalize(cameraForward + rawOffsetDirection);

				Reference<Transform> closest;
				float closestDistance = std::numeric_limits<float>::infinity();
				for (size_t i = 0; i < self->m_arrowTransforms.size(); i++) {
					Transform* axisTransform = self->m_arrowTransforms[i];
					if (!axisTransform->Enabled()) continue;
					const Vector3 axisDirection = Math::Normalize(axisTransform->LocalPosition());
					const Vector3 handlePosition = (axisDirection * AxisHandleCenterOffset());
					const Vector3 offset = (handlePosition - origin);
					const float projectionSize = Math::Dot(offset, direction);
					const float normalSize = Math::Magnitude(offset - (direction * projectionSize));
					if ((normalSize < AxisHandleHandleRadius()) && (closest == nullptr || closestDistance > projectionSize)) {
						closest = axisTransform;
						closestDistance = projectionSize;
					}
				}

				if (closest != nullptr) 
					OnArrowClicked(closest, dynamic_cast<Viewport*>(self->m_viewport.operator->()));
				else {
					const float distance = Math::Magnitude(origin);
					const Vector3 centerOffset = distance * rawOffsetDirection;
					const float offsetAmount = Math::Magnitude(centerOffset);
					if (offsetAmount < CenterRadius())
						CenterClicked(self);
				}
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
					const Reference<RenderStack::Renderer> renderer = ForwardLightingModel_Opaque_Pass::Instance()->CreateRenderer(
						self->m_viewport,
						LayerMask::All(),
						Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH |
						Graphics::RenderPass::Flags::RESOLVE_COLOR);
					self->m_renderStack->AddRenderer(renderer);
				}

				// Create sphere in the center:
				{
					const Reference<Transform> transform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), "Central Sphere");
					transform->SetLocalScale(Vector3(CenterRadius()));
					const Reference<TriMesh> mesh = MeshConstants::Tri::Sphere();
					Object::Instantiate<MeshRenderer>(transform, "Central Sphere Renderer", mesh);
				}

				// Create 'arrows':
				{
					auto createArrow = [&](const Vector3& direction) {
						std::stringstream name;
						name << "Axis " << direction;
						const Reference<Transform> transform = Object::Instantiate<Transform>(self->m_subscene->Context()->RootObject(), name.str());
						transform->SetLocalScale(Vector3(0.25f, 0.5f, 0.25f));
						transform->SetWorldPosition(direction * (AxisHandleCenterOffset() - 0.25f));
						name << " Renderer";
						const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(self->Context()->Graphics()->Device(), direction);
						Object::Instantiate<MeshRenderer>(transform, name.str(), MeshConstants::Tri::Cone())->SetMaterialInstance(material);
						self->m_arrowTransforms.push_back(transform);
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
						transform->SetLocalScale(Vector3(0.25f));
						transform->SetWorldPosition(direction * -AxisHandleCenterOffset());
						name << " Renderer";
						const Reference<const Material::Instance> material = SampleDiffuseShader::MaterialInstance(self->Context()->Graphics()->Device(), direction);
						Object::Instantiate<MeshRenderer>(transform, name.str(), MeshConstants::Tri::Cube())->SetMaterialInstance(material);
						self->m_arrowTransforms.push_back(transform);
						return transform;
					};
					createNegArrow(Math::Right())->SetWorldEulerAngles(Vector3(0.0f, 0.0f, 90.0f));
					createNegArrow(Math::Up());
					createNegArrow(Math::Forward())->SetWorldEulerAngles(Vector3(-90.0f, 0.0f, 0.0f));
				}
			}

			static void DestructSubscene(SceneViewAxisVectors* self) {
				self->m_arrowTransforms.clear();
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
				createArgs.logic.input = Object::Instantiate<OS::NoInput>();
				createArgs.logic.assetDatabase = context->AssetDB();
			}
			{
				createArgs.graphics.graphicsDevice = context->Graphics()->Device();
				createArgs.graphics.shaderLoader = context->Graphics()->Configuration().ShaderLoader();
				createArgs.graphics.maxInFlightCommandBuffers = context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
				createArgs.graphics.bindlessResources.bindlessArrays = context->Graphics()->Bindless().Buffers();
				createArgs.graphics.bindlessResources.bindlessArrayBindings = context->Graphics()->Bindless().BufferBinding();
				createArgs.graphics.bindlessResources.bindlessSamplers = context->Graphics()->Bindless().Samplers();
				createArgs.graphics.bindlessResources.bindlessSamplerBindings = context->Graphics()->Bindless().SamplerBinding();
				createArgs.graphics.synchPointThreadCount = 1;
				createArgs.graphics.renderThreadCount = 1;
			}
			{
				createArgs.physics.physicsInstance = context->Physics()->APIInstance();
				createArgs.physics.simulationThreadCount = 1u;
				createArgs.physics.sceneFlags = Physics::PhysicsInstance::SceneCreateFlags::NONE;
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
			else {
				while (m_subscene->Context()->Graphics()->InFlightCommandBufferIndex() != context->Graphics()->InFlightCommandBufferIndex())
					m_subscene->SynchAndRender(context->Time()->UnscaledDeltaTime());
				Tools::ConstructSubscene(this);
			}
			Context()->Graphics()->OnGraphicsSynch() += Callback(&Tools::UpdateSubscene, this);
		}

		SceneViewAxisVectors::~SceneViewAxisVectors() {}

		void SceneViewAxisVectors::OnComponentDestroyed() {
			std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
			Context()->Graphics()->OnGraphicsSynch() -= Callback(&Tools::UpdateSubscene, this);
			Tools::DestructSubscene(this);
			m_renderStack = nullptr;
			m_subscene = nullptr;
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
				const ImVec2 drawPosition = ImVec2(
					ImGui::GetWindowContentRegionMax().x - imageSize.x, 
					ImGui::GetWindowContentRegionMin().y);
				ImGui::SetCursorPos(drawPosition);
				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				if (ImGui::ImageButton(m_guiTexture->operator ImTextureID(), imageSize)) {
					const ImVec2 windowPosition = ImGui::GetWindowPos();
					const ImVec2 mousePosition = ImGui::GetMousePos();
					const Size2 clickedPos(
						static_cast<uint32_t>(mousePosition.x - drawPosition.x - windowPosition.x + 1.0f),
						static_cast<uint32_t>(mousePosition.y - drawPosition.y - windowPosition.y));
					Tools::OnClick(this, clickedPos, Size2(static_cast<uint32_t>(imageSize.x), static_cast<uint32_t>(imageSize.y)));
				}
				ImGui::PopStyleColor(3);
				ImGui::PopStyleVar();
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
