#pragma once
#include "../DrawSerializedObject.h"
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>

namespace Jimara {
	namespace Editor {
		/// <summary> EnumAttributeDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::EnumAttributeDrawer);

		/// <summary>
		/// Drawer for integer/float serialized objects with EnumAttribute
		/// </summary>
		class EnumAttributeDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as enumeration value (compatible only with value types)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_REFERENCE_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a EnumAttribute) </param>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::EnumAttributeDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::EnumAttributeDrawer>();
}
