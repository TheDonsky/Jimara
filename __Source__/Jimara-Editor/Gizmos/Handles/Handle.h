#pragma once
#include "../Gizmo.h"


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
			~Handle() = 0;

			/// <summary> True, if the handle is currently being manipulated </summary>
			bool HandleActive()const;

			/// <summary> True, if the handle is hovered on or active </summary>
			bool HandleHovered()const;

			/// <summary> Invoked when handle starts being hovered on </summary>
			Event<Handle*>& OnCursorEntered()const;

			/// <summary> Invoked when handle starts being dragged </summary>
			Event<Handle*>& OnHandleActivated()const;

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			Event<Handle*>& OnHandleUpdated()const;

			/// <summary> Invoked when handle stops being dragged </summary>
			Event<Handle*>& OnHandleDeactivated()const;

			/// <summary> Invoked when handle stops being hovered on </summary>
			Event<Handle*>& OnCursorRemoved()const;

			/// <summary>
			/// Often, handles will be used for dragging stuff around and changing gizmo targets;
			/// In those cases, for Undo to work, it's adviced to subscribe this method to OnHandleDeactivated() event
			/// <para/> Note: This will automatically happen right after OnHandleDeactivated(), 
			///		as long as the handle is a child object of the gizmo and selection does not get medddled with.
			/// With Callback<Handle*>(Handle::TrackGizmoTargets, gizmo) call
			/// </summary>
			/// <param name="gizmo"> Target gizmo </param>
			/// <param name=""> Ignored handle that emitted the event </param>
			inline static void TrackGizmoTargets(Gizmo* gizmo, Handle*) { gizmo->TrackTargets(false); }

			/// <summary>
			/// Short for Callback<Handle*>(Handle::TrackGizmoTargets, gizmo);
			/// </summary>
			/// <param name="gizmo"> Target gizmo </param>
			/// <returns> Callback<Handle*> to subscribe to OnHandleDeactivated() </returns>
			inline static Callback<Handle*> TrackGizmoTargetsCallback(Gizmo* gizmo) { return Callback<Handle*>(Handle::TrackGizmoTargets, gizmo); }

			/// <summary> Gizmo context </summary>
			GizmoScene::Context* GizmoContext()const;

		protected:
			/// <summary> Invoked when handle starts being hovered on (before OnCursorEntered()) </summary>
			inline virtual void CursorEntered() {}

			/// <summary> Invoked, if the handle is starts being dragged (before OnHandleActivated()) </summary>
			inline virtual void HandleActivated() {}

			/// <summary> Invoked, if the handle is dragged (before OnHandleUpdated()) </summary>
			inline virtual void UpdateHandle() {}

			/// <summary> Invoked when handle stops being dragged (before OnHandleDeactivated()) </summary>
			inline virtual void HandleDeactivated() {}

			/// <summary> Invoked when handle stops being hovered on (before OnCursorRemoved()) </summary>
			inline virtual void CursorRemoved() {}

		private:
			// Activation and update events
			mutable EventInstance<Handle*> m_onCursorEnter;
			mutable EventInstance<Handle*> m_onHandleActivated;
			mutable EventInstance<Handle*> m_onHandleUpdated;
			mutable EventInstance<Handle*> m_onHandleDeactivated;
			mutable EventInstance<Handle*> m_onCursorExit;

			// Active state
			bool m_active = false;

			// Hovered state
			bool m_hovered = false;

			// Context
			mutable Reference<GizmoScene::Context> m_context;

			// Updater & Selector gizmo
			class HandleSelector;

			// Well well well..
			friend class Jimara::TypeIdDetails;
		};
	}
}
