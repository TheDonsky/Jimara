#pragma once
#include "../EditorWindow.h"
#include <Data/Formats/ConfigurableResourceFileAsset.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Add a record in registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::ConfigurableResourceInspector);

		/// <summary>
		/// Editor window for configurable resource settings
		/// </summary>
		class ConfigurableResourceInspector : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			ConfigurableResourceInspector(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~ConfigurableResourceInspector();

			/// <summary> Target resource </summary>
			ConfigurableResource* Target()const;

			/// <summary>
			/// Updates target resource
			/// </summary>
			/// <param name="resource"> Configurable resource to edit </param>
			void SetTarget(ConfigurableResource* resource);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Target resource
			Reference<ConfigurableResource> m_target;

			// Last unmodified snapshot
			std::optional<nlohmann::json> m_initialSnapshot;

			// Private stuff is in here
			struct Helpers;
		};
	}

	// TypeIdDetails for ConfigurableResourceInspector
	template<> void TypeIdDetails::GetParentTypesOf<Editor::ConfigurableResourceInspector>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::ConfigurableResourceInspector>(const Callback<const Object*>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::ConfigurableResourceInspector>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::ConfigurableResourceInspector>();
}
