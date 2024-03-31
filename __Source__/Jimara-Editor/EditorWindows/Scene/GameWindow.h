#pragma once
#include "../../Environment/JimaraEditor.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the registry know about GameWindow </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::GameWindow);

		/// <summary>
		/// Draws whatever the game renderer will render to an external window
		/// </summary>
		class GameWindow : public virtual Object {
		public:
			/// <summary>
			/// Creates a new GameWindow
			/// </summary>
			/// <param name="context"> Editor context </param>
			static void Create(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~GameWindow();

		private:
			const Reference<EditorContext> m_context;
			const Reference<OS::Window> m_window;
			const Reference<Graphics::RenderSurface> m_surface;
			const Reference<Graphics::RenderEngine> m_surfaceRenderEngine;

			GameWindow(EditorContext* context, OS::Window* window, 
				Graphics::RenderSurface* surface, Graphics::RenderEngine* surfaceRenderEngine);

			struct Helpers;
		};
	}

	// TypeIdDetails for GameView
	template<> void TypeIdDetails::GetParentTypesOf<Editor::GameWindow>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::GameWindow>(const Callback<const Object*>& report);
}
