#pragma once
#include "../EditorWindow.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Add a record in registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::AssetBrowser);

		/// <summary>
		/// Editor window for browsing assets folder
		/// </summary>
		class AssetBrowser : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			AssetBrowser(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~AssetBrowser();

			/// <summary> Active [sub]directory </summary>
			inline OS::Path& ActiveDirectory() { return m_currentSubdirectory; }

			/// <summary> Active [sub]directory </summary>
			inline const OS::Path& ActiveDirectory()const { return m_currentSubdirectory; }

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Current directory
			OS::Path m_currentSubdirectory = "";

			// Current selection
			std::string m_currentSelection = "";

			// Private stuff is in here
			struct Helpers;
		};
	}

	// TypeIdDetails for AssetBrowser
	template<> void TypeIdDetails::GetParentTypesOf<Editor::AssetBrowser>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::AssetBrowser>(const Callback<const Object*>& report);
}

