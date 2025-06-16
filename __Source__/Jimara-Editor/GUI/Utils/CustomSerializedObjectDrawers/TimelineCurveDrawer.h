#pragma once
#include "../DrawSerializedObject.h"


namespace Jimara {
	namespace Editor {
		/// <summary> TimelineCurveDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TimelineCurveDrawer);

		/// <summary>
		/// Drawer TimelineCurve serialized objects
		/// </summary>
		class TimelineCurveDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as TimelineCurve graph (compatible only with TimelineCurve<> values of limited types)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_REFERENCE_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (TimelineCurve::EditableTimelineCurveAttribute) </param>
			/// <returns> True, when the underlying field modification ends </returns>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;

			/// <summary> Singleton instance of a TimelineCurveDrawer </summary>
			static const TimelineCurveDrawer* Instance();

		private:
			// Most of the implementation resides here..
			struct Helpers;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TimelineCurveDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TimelineCurveDrawer>();
}

