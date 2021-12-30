#pragma once
#include "../DrawSerializedObject.h"
#include <Data/Serialization/Attributes/SliderAttribute.h>

namespace Jimara {
	namespace Editor {
		/// <summary> SliderAttributeDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::SliderAttributeDrawer);

		/// <summary>
		/// Drawer for integer/float serialized objects with SliderAttribute
		/// </summary>
		class SliderAttributeDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as slider value (compatible only with integer/float values)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a SliderAttribute) </param>
			virtual void DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Callback<const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::SliderAttributeDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::SliderAttributeDrawer>();
}
