#pragma once
#include "../EditorWindow.h"
#include <Jimara/Data/Formats/PhysicsMaterialFileAsset.h>

namespace Jimara {
	namespace Editor {
		/// <summary> Add a record in registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::PhysicsMaterialInspector);

		/// <summary>
		/// Editor window for Physics Material settings
		/// </summary>
		class PhysicsMaterialInspector : public virtual EditorWindow {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Editor context </param>
			PhysicsMaterialInspector(EditorContext* context);

			/// <summary> Virtual destructor </summary>
			virtual ~PhysicsMaterialInspector();

			/// <summary> Target physics material </summary>
			Physics::PhysicsMaterial* Target()const;

			/// <summary>
			/// Updates target physics material
			/// </summary>
			/// <param name="material"> Physics material to edit </param>
			void SetTarget(Physics::PhysicsMaterial* material);

		protected:
			/// <summary> Draws Editor window </summary>
			virtual void DrawEditorWindow() final override;

		private:
			// Target material
			Reference<Physics::PhysicsMaterial> m_target;

			// Last unmodified snapshot
			std::optional<nlohmann::json> m_initialSnapshot;

			// Private stuff is in here
			struct Helpers;
		};
	}

	// TypeIdDetails for PhysicsMaterialInspector
	template<> void TypeIdDetails::GetParentTypesOf<Editor::PhysicsMaterialInspector>(const Callback<TypeId>& report);
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::PhysicsMaterialInspector>(const Callback<const Object*>& report);
	template<> void TypeIdDetails::OnRegisterType<Editor::PhysicsMaterialInspector>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::PhysicsMaterialInspector>();
}
