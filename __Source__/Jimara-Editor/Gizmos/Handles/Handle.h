#pragma once
#include <Environment/Scene/Scene.h>


namespace Jimara {
	namespace Editor {
		class Handle;
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::Handle>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::Handle>();

	namespace Editor {
		/// <summary> Let the system know about Hadles </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::Handle);

		/// <summary>
		/// Generic handle component, implementations of which incluse draggable arrows, on-screen rotation controls and similar stuff
		/// </summary>
		class Handle : public virtual Component {
		public:
			/// <summary> Constructor </summary>
			Handle();

			/// <summary> Virtual destructor </summary>
			virtual ~Handle();

			/// <summary> True, if the handle is currently being manipulated </summary>
			bool IsActive()const;

			/// <summary> Invoked when handle starts being dragged </summary>
			Event<Handle*>& OnHandleActivated()const;

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			Event<Handle*>& OnHandleUpdated()const;

			/// <summary> Invoked when handle stops being dragged </summary>
			Event<Handle*>& OnHandleDeactivated()const;

		protected:
			/// <summary> Invoked, if the handle is dragged </summary>
			virtual void UpdateHandle() = 0;

		private:
			// Activation and update events
			mutable EventInstance<Handle*> m_onHandleActivated;
			mutable EventInstance<Handle*> m_onHandleUpdated;
			mutable EventInstance<Handle*> m_onHandleDeactivated;

			// Active state
			bool m_active = false;

			// Updater & Selector gizmo
			class HandleSelector;

			// Well well well..
			friend class Jimara::TypeIdDetails;
		};
	}
}
