#pragma once
#include "../DrawSerializedObject.h"
#include <Jimara/Data/Serialization/Attributes/TextBoxAttribute.h>


namespace Jimara {
	namespace Editor {
		/// <summary> Let the system know about the attribute drawer </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::TextBoxAttributeDrawer);

		/// <summary>
		/// Drawer for text boxes
		/// </summary>
		class TextBoxAttributeDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as text box (compatible only with string-type values)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_REFERENCE_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a TextBoxAttribute) </param>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TextBoxAttributeDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::TextBoxAttributeDrawer>();
}
