#pragma once
#include "../DrawSerializedObject.h"
#include <Data/Serialization/Attributes/ColorAttribute.h>

namespace Jimara {
	namespace Editor {
		/// <summary> ColorAttributeDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::ColorAttributeDrawer);

		/// <summary>
		/// Drawer for Vector3/Vector4 serialized objects with ColorAttribute
		/// </summary>
		class ColorAttributeDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as color value (compatible only with Vector3/Vector4 values)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a ColorAttribute) </param>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::ColorAttributeDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::ColorAttributeDrawer>();
}
