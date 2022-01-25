#include "GameView.h"


namespace Jimara {
	namespace Editor {
		GameView::GameView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Game View") {
			std::stringstream stream;
			stream << "Game View###game_view_" << ((size_t)this);
			EditorWindowName() = stream.str();
		}

		void GameView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
			const Vector2 viewport = toVec2(ImGui::GetItemRectSize());
			const Vector2 windowStart = toVec2(ImGui::GetWindowPos()) + Jimara::Vector2(0.0f, viewport.y);
			const Vector2 windowSize = toVec2(ImGui::GetWindowSize()) - Jimara::Vector2(0.0f, viewport.y);
			const Vector2 windowEnd = (windowStart + windowSize);
			editorScene->RequestResolution(Size2((uint32_t)windowSize.x, (uint32_t)windowSize.y));
			Reference<Graphics::TextureView> texture = editorScene->RootObject()->Context()->Graphics()->Renderers().TargetTexture();
			if (texture != nullptr)
				ImGuiRenderer::Texture(texture->TargetTexture(), Jimara::Rect(windowStart, windowEnd));
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/GameView", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<GameView>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::GameView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::GameView>() {
		Editor::action = nullptr;
	}
}
