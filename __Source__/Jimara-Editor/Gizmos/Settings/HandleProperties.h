#pragma once
#include "../GizmoScene.h"

namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know HandleProperties exist </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::HandleProperties);

		/// <summary>
		/// General shared handle properties
		/// </summary>
		class HandleProperties : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		public:
			/// <summary>
			/// Retrieves common instance of HandleProperties for given Editor context
			/// </summary>
			/// <param name="context"> Editor application context </param>
			/// <returns> Shared HandleProperties instance </returns>
			static Reference<HandleProperties> Of(EditorContext* context);

			/// <summary> Preffered base handle size in pixels </summary>
			inline float HandleSize()const { return m_handleSize; }

			/// <summary>
			/// Sets base handle size
			/// </summary>
			/// <param name="value"> New size to use (in pixels) </param>
			inline void SetHandleSize(float value) { m_handleSize = value; }

			/// <summary>
			/// Calculates preffered base handles size in world units for given gizmo scene viewport and given world-space placement location
			/// </summary>
			/// <param name="viewport"> Gizmo scene viewport </param>
			/// <param name="position"> World-space location </param>
			/// <returns> World space size/scale for gizmo at the location </returns>
			float HandleSizeFor(const GizmoViewport* viewport, const Vector3& position);

			/// <summary> Priority of corresponding GizmoGUI::Drawer </summary>
			static float GizmoGUIPriority();

		private:
			// Handle size
			std::atomic<float> m_handleSize = 128.0f;

			// Cache of instances
			class Cache;

			// Constructor needs to be private
			inline HandleProperties() {}
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::HandleProperties>(const Callback<const Object*>& report);
}

