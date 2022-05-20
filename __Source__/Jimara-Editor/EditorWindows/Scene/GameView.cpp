#include "GameView.h"
#include "../../Environment/EditorStorage.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>


namespace Jimara {
	namespace Editor {
		GameView::GameView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Game View") {}

		namespace {
			inline static void DrawRenderedImage(EditorScene* editorScene,
				Reference<Graphics::TextureView>& lastImage, Reference<ImGuiTexture>& lastGUITexture) {
				auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
				const ImGuiStyle& style = ImGui::GetStyle();
				const Vector2 viewport = toVec2(ImGui::GetCursorPos()) * Vector2(0.0f, 1.0f) + Vector2(style.WindowBorderSize, 0.0f);
				const Vector2 windowSize = toVec2(ImGui::GetWindowSize()) - viewport - Vector2(style.WindowBorderSize);
				if (windowSize.x < 0.0f || windowSize.y <= 0.0f) return;

				editorScene->RequestResolution(Size2((uint32_t)windowSize.x, (uint32_t)windowSize.y));
				Reference<RenderImages> images = RenderStack::Main(editorScene->RootObject()->Context())->Images();
				Reference<Graphics::TextureView> texture = (images == nullptr) ? nullptr : images->GetImage(RenderImages::MainColor())->Resolve();
				
				auto fail = [&]() {
					lastImage = nullptr;
					lastGUITexture = nullptr;
					return;
				};
				if (texture == nullptr || (windowSize.x * windowSize.y) <= 0.0f) return fail();

				const Vector2 textureSize = [&]() {
					const Size3 size = texture->TargetTexture()->Size();
					return Vector2(size.x, size.y);
				}();
				if ((textureSize.x * textureSize.y) <= 0.0f) return fail();

				Vector2 windowStart = toVec2(ImGui::GetWindowPos()) + viewport;
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

				if (lastImage != texture) {
					const auto sampler = texture->CreateSampler();
					lastImage = texture;
					lastGUITexture = ImGuiRenderer::Texture(sampler);
				}
				auto toImVec = [](auto vec) { return ImVec2(vec.x, vec.y); };
				ImGui::GetWindowDrawList()->AddImage(*lastGUITexture, toImVec(windowStart), toImVec(windowEnd));
				
				if (ImGui::IsWindowFocused()) {
					const Size3 size = texture->TargetTexture()->Size();
					const float textureAspect = static_cast<float>(size.x) / static_cast<float>(size.y);
					const float windowAspect = windowSize.x / windowSize.y;
					const Rect subRect = [&]() -> Rect {
						if (textureAspect > windowAspect) {
							// Texture wider than window:
							const float drawnHeight = (windowSize.x / textureAspect);
							const float deltaHeight = 0.5f * (windowSize.y - drawnHeight);
							return Rect(Vector2(0.0f, deltaHeight), Vector2(windowSize.x, windowSize.y - deltaHeight));
						}
						else {
							// Window wider than texture:
							const float drawnWidth = (windowSize.y * textureAspect);
							const float deltaWidth = 0.5f * (windowSize.x - drawnWidth);
							return Rect(Vector2(deltaWidth, 0.0f), Vector2(windowSize.x - deltaWidth, windowSize.y));
						}
					}();
					editorScene->RequestInputOffsetAndScale(windowStart + subRect.start, static_cast<float>(size.x) / subRect.Size().x);
				}
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
				ImGui::Separator();
			}
		}

		void GameView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			DrawPlayStateButtons(editorScene, this);
			DrawRenderedImage(editorScene, m_lastImage, m_lastSampler);
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/GameView", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<GameView>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}

		namespace {
			class GameViewSerializer : public virtual EditorStorageSerializer::Of<GameView> {
			public:
				inline GameViewSerializer() : Serialization::ItemSerializer("GameView", "Game View (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, GameView* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::GameView>(const Callback<const Object*>& report) {
		static const Editor::GameViewSerializer instance;
		report(&instance);
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::GameView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::GameView>() {
		Editor::action = nullptr;
	}
}
