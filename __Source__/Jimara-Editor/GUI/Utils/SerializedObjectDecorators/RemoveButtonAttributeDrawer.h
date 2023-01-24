#pragma once
#include "../DrawSerializedObject.h"


namespace Jimara {
	namespace Editor {
		/// <summary> RemoveButtonAttributeDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::RemoveButtonAttributeDrawer);

		/// <summary>
		/// Drawer for integer/float serialized objects with EnumAttribute
		/// </summary>
		class RemoveButtonAttributeDrawer : public virtual SerializedObjectDecoratorDrawer {
		public:
			/// <summary> Constructor </summary>
			RemoveButtonAttributeDrawer();

			/// <summary>
			/// Draws a SerializedObject with remove button attribute
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (RemoveButtonAttribute) </param>
			virtual bool DecorateObject(const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::RemoveButtonAttributeDrawer>(const Callback<const Object*>& report);
}
