#include "SceneViewAxisVectors.h"
#include <OS/Input/NoInput.h>

namespace Jimara {
	namespace Editor {
		struct SceneViewAxisVectors::Tools {
			static void UpdateSubscene(SceneViewAxisVectors* self) {
				self->m_subscene->Update(self->Context()->Time()->UnscaledDeltaTime());
			}

			static void ConstructSubscene(SceneViewAxisVectors* self) {
				// __TODO__: Implement this crap!
				// __TODO__: Create Arrows!
				// __TODO__: Create Light!
				// __TODO__: Create Viewport!
				// __TODO__: Create Renderer!
			}
		};

		SceneViewAxisVectors::SceneViewAxisVectors(Scene::LogicContext* context)
			: Component(context, "SceneViewAxisVectors")
			, m_subscene([&]() ->Reference<Scene> {
			Scene::CreateArgs createArgs = {};
			{
				createArgs.logic.logger = context->Log();
				createArgs.logic.input = Object::Instantiate<OS::NoInput>();
				createArgs.logic.assetDatabase = context->AssetDB();
			}
			{
				createArgs.graphics.graphicsDevice = context->Graphics()->Device();
				createArgs.graphics.shaderLoader = context->Graphics()->Configuration().ShaderLoader();
				EditorContext::SceneLightTypes lightTypes = GizmoScene::GetContext(context)->EditorApplicationContext()->LightTypes();
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
			Context()->Graphics()->OnGraphicsSynch() += Callback(&Tools::UpdateSubscene, this);
			if (m_subscene == nullptr)
				Context()->Log()->Error("SceneViewAxisVectors - Failed to create subscene for corner arrows!");
			else Tools::ConstructSubscene(this);
		}

		SceneViewAxisVectors::~SceneViewAxisVectors() {
			Context()->Graphics()->OnGraphicsSynch() -= Callback(&Tools::UpdateSubscene, this);
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
				{
					const ImVec2 windowPos = ImGui::GetWindowPos();
					ImDrawList* const draw_list = ImGui::GetWindowDrawList();
					ImVec2 rectStart = ImVec2(windowPos.x + drawPosition.x, windowPos.y + drawPosition.y);
					draw_list->AddRectFilled(rectStart, ImVec2(rectStart.x + imageSize.x, rectStart.y + imageSize.y), IM_COL32(200, 200, 200, 100));
				}
				ImGui::SetCursorPos(drawPosition);
				ImGui::Image(m_guiTexture->operator ImTextureID(), imageSize);
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
