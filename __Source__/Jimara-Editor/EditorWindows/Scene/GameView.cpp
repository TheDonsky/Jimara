#include "GameView.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>


namespace Jimara {
	namespace Editor {
		GameView::GameView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Game View") {
			std::stringstream stream;
			stream << "Game View###game_view_" << ((size_t)this);
			EditorWindowName() = stream.str();
		}

		namespace {
			inline static void DrawRenderedImage(EditorScene* editorScene) {
				auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
				const Vector2 viewport = toVec2(ImGui::GetItemRectSize());
				const Vector2 windowSize = toVec2(ImGui::GetWindowSize()) - Jimara::Vector2(0.0f, viewport.y);

				editorScene->RequestResolution(Size2((uint32_t)windowSize.x, (uint32_t)windowSize.y));
				Reference<Graphics::TextureView> texture = editorScene->RootObject()->Context()->Graphics()->Renderers().TargetTexture();
				if (texture == nullptr || (windowSize.x * windowSize.y) <= 0.0f) return;

				const Vector2 textureSize = [&]() {
					const Size3 size = texture->TargetTexture()->Size();
					return Vector2(size.x, size.y);
				}();
				if ((textureSize.x * textureSize.y) <= 0.0f) return;

				Vector2 windowStart = toVec2(ImGui::GetWindowPos()) + Jimara::Vector2(0.0f, viewport.y);
				Vector2 windowEnd = (windowStart + windowSize);

				const float windowAspect = (windowSize.x / windowSize.y);
				const float textureAspect = (textureSize.x / textureSize.y);

				if (windowAspect > textureAspect) {
					float widthDiff = (windowSize.x - (textureAspect * windowSize.y)) * 0.5f;
					windowStart.x += widthDiff;
					windowEnd.x -= widthDiff;
				}
				else {
					float heightDiff = (windowSize.y - (windowSize.x / textureAspect)) * 0.5f;
					windowStart.y += heightDiff;
					windowEnd.y -= heightDiff;
				}

				ImGuiRenderer::Texture(texture->TargetTexture(), Jimara::Rect(windowStart, windowEnd));
			}

			static ImVec2 PlayButtonSize() { return ImVec2(32.0f, 16.0f); }

			inline static void DrawPlayButton(EditorScene* scene, GameView* view) {
				std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_PLAY << "###editor_game_view_" << ((size_t)view) << "_play_button";
					return stream.str();
				}();
				if (ImGui::Button(text.c_str(), PlayButtonSize()))
					scene->Play();
			}
			
			static ImVec2 PauseButtonSize() { return PlayButtonSize(); }

			inline static void DrawPauseButton(EditorScene* scene, GameView* view) {
				std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_PAUSE << "###editor_game_view_" << ((size_t)view) << "_pause_button";
					return stream.str();
				}();
				if (ImGui::Button(text.c_str(), PauseButtonSize()))
					scene->Pause();
			}

			static ImVec2 StopButtonSize() { return PlayButtonSize(); }

			inline static void DrawStopButton(EditorScene* scene, GameView* view) {
				std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_STOP << "###editor_game_view_" << ((size_t)view) << "_stop_button";
					return stream.str();
				}();
				if (ImGui::Button(text.c_str(), StopButtonSize()))
					scene->Stop();
			}

			inline static void DrawPlayStateButtons(EditorScene* scene, GameView* view) {
				EditorScene::PlayState state = scene->State();
				ImGui::SetCursorPosX(
					(ImGui::GetWindowSize().x -
						((state != EditorScene::PlayState::PLAYING) ? PlayButtonSize().x : PauseButtonSize().x) -
						((state != EditorScene::PlayState::STOPPED) ? StopButtonSize().x : 0.0f)) * 0.5f);
				if (state != EditorScene::PlayState::PLAYING)
					DrawPlayButton(scene, view);
				else DrawPauseButton(scene, view);
				if (state != EditorScene::PlayState::STOPPED) {
					ImGui::SameLine();
					DrawStopButton(scene, view);
				}
			}
		}

		void GameView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			DrawRenderedImage(editorScene);
			DrawPlayStateButtons(editorScene, this);
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
