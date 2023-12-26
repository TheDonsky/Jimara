#pragma once
#include "../EditorWindow.h"
#include <Data/Formats/MaterialFileAsset.h>

namespace Jimara {
	namespace Editor {
		/// <summary> Add a record in registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::MaterialInspector);

		/// <summary>
		/// Editor window for Material settings
		/// </summary>
		class MaterialInspector : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			MaterialInspector(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~MaterialInspector();

			/// <summary> Target material </summary>
			Material* Target()const;

			/// <summary>
			/// Updates target material
			/// </summary>
			/// <param name="material"> Material to edit </param>
			void SetTarget(Material* material);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Target material
			Reference<Material> m_target;

			// Last unmodified snapshot
			std::optional<nlohmann::json> m_initialSnapshot;

			// Mini-scene for the display
			Reference<Scene> m_displayScene;
			Reference<ImGuiTexture> m_displayTexture;
			Reference<Graphics::TextureView> m_displayView;
			std::atomic<uint32_t> m_numRequiredRenders = 8u;

			// Private stuff is in here
			struct Helpers;
		};
	}

	// TypeIdDetails for MaterialInspector
	template<> void TypeIdDetails::GetParentTypesOf<Editor::MaterialInspector>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MaterialInspector>(const Callback<const Object*>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::MaterialInspector>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::MaterialInspector>();
}
