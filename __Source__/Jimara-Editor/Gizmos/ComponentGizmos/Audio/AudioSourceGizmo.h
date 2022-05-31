#pragma once
#include "../../Gizmo.h"


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about our class </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::AudioSourceGizmo);

		/// <summary>
		/// Gizmo for audio sources
		/// </summary>
		class AudioSourceGizmo : public virtual Gizmo, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary> Subtype for 2d sources </summary>
			class Source2D;

			/// <summary> Subtype for 3d sources </summary>
			class Source3D;

			/// <summary> Virtual destructor </summary>
			virtual ~AudioSourceGizmo() = 0;

		protected:
			/// <summary> Engine update callback </summary>
			virtual void Update()override;

		private:
			// Underlying transform for visuals
			Reference<Transform> m_transform;

			// Some helper functions
			struct Helpers;

			// Only Source2D and Source3D can access the constructor
			AudioSourceGizmo();
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::AudioSourceGizmo>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::AudioSourceGizmo>();
}
