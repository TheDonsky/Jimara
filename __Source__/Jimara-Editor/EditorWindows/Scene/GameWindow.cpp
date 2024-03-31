#include "GameWindow.h"


namespace Jimara {
	namespace Editor {
		struct GameWindow::Helpers {
			struct Renderer : public virtual Graphics::ImageRenderer {
				const Reference<EditorContext> context;

				inline Renderer(EditorContext* ctx) : context(ctx) {}

				inline virtual ~Renderer() {}

				virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override {
					return engineInfo;
				}

				virtual void Render(Object* engineData, Graphics::InFlightBufferInfo bufferInfo) override {
					const Reference<EditorScene> scene = context->GetScene();
					if (scene == nullptr)
						return;

					Graphics::RenderEngineInfo* engineInfo = dynamic_cast<Graphics::RenderEngineInfo*>(engineData);
					if (engineInfo == nullptr)
						return;
					
					const Reference<RenderStack> renderStack = RenderStack::Main(scene->RootObject()->Context());
					if (renderStack == nullptr)
						return;

					const Reference<RenderImages> images = renderStack->Images();
					if (images == nullptr)
						return;

					RenderImages::Image* const mainColor = images->GetImage(RenderImages::MainColor());
					if (mainColor == nullptr)
						return;

					engineInfo->Image(bufferInfo)->Blit(bufferInfo, mainColor->Resolve()->TargetTexture());
				}
			};

			static void Update(GameWindow* self) {
				Reference<GameWindow> selfRef(self);
				if (self->m_window->Closed()) {
					self->m_context->RemoveStorageObject(self);
					return;
				}

				Reference<EditorScene> scene = self->m_context->GetScene();
				if (scene == nullptr) {
					scene = Object::Instantiate<EditorScene>(self->m_context);
					self->m_context->SetScene(scene);
				}

				scene->RequestResolution(self->m_window->FrameBufferSize());

				// __TODO__: If the window is focused, request offset & scale!

				// __TODO__: Implement this crap!

				self->m_surfaceRenderEngine->Update();
			}
		};

		void GameWindow::Create(EditorContext* context) {
			if (context == nullptr)
				return;
			context->AddRef();
			typedef void(*CreateFn)(EditorContext*);
			static const CreateFn createFn = [](EditorContext* context) {
				if (context == nullptr)
					return;
				Reference<EditorContext> ctx(context);
				context->ReleaseRef();

				auto fail = [&](const auto&... message) {
					context->Log()->Error("GameWindow::Create - ", message...);
				};

				const Reference<OS::Window> window = OS::Window::Create(context->Log(), "Game Window");
				if (window == nullptr)
					return fail("Failed to create a window! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::RenderSurface> surface = context->GraphicsDevice()->GraphicsInstance()->CreateRenderSurface(window);
				if (surface == nullptr)
					return fail("Failed to create render surface! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::RenderEngine> renderEngine = context->GraphicsDevice()->CreateRenderEngine(surface);
				if (renderEngine == nullptr)
					return fail("Failed to create surface render engine! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Helpers::Renderer> renderer = Object::Instantiate<Helpers::Renderer>(context);
				renderEngine->AddRenderer(renderer);

				const Reference<GameWindow> gameWindow = new GameWindow(context, window, surface, renderEngine);
				context->AddStorageObject(gameWindow);
				context->OnMainLoop() += Callback(Helpers::Update, gameWindow.operator->());
				gameWindow->ReleaseRef();
			};
			std::thread(createFn, context).detach();
		}

		GameWindow::~GameWindow() {
			m_context->OnMainLoop() -= Callback(Helpers::Update, this);
		}

		GameWindow::GameWindow(
			EditorContext* context, OS::Window* window,
			Graphics::RenderSurface* surface, Graphics::RenderEngine* surfaceRenderEngine)
			: m_context(context)
			, m_window(window)
			, m_surface(surface)
			, m_surfaceRenderEngine(surfaceRenderEngine) {
			m_context->OnMainLoop() += Callback(Helpers::Update, this);
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameWindow>(const Callback<TypeId>& report) {
		report(TypeId::Of<Object>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::GameWindow>(const Callback<const Object*>& report) {
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Scene/GameWindow", "Open Game Window (displays game output)", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Editor::GameWindow::Create(context);
				}));
		// Performance is terrible when we have more than a single window open, so I decided to disable the button for now..
		//report(&editorMenuCallback);
	}
}
