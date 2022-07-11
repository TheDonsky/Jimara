#pragma once
#include "../DrawSerializedObject.h"


namespace Jimara {
	namespace Editor {
		/// <summary> LayerDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::LayerDrawer);

		/// <summary>
		/// Drawer for Layer, with Layers::LayerAttribute
		/// </summary>
		class LayerDrawer : public virtual CustomSerializedObjectDrawer {
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
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a Layers::LayerAttribute) </param>
			/// <returns> True, when the underlying field modification ends </returns>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};

		/// <summary> LayerDrawer is supposed to be registered through Editor's main type registry </summary>
		JIMARA_REGISTER_TYPE(Jimara::Editor::LayerMaskDrawer);

		/// <summary>
		/// Drawer for LayerMask, with Layers::LayerMaskAttribute
		/// </summary>
		class LayerMaskDrawer : public virtual CustomSerializedObjectDrawer {
		public:
			/// <summary>
			/// Draws a SerializedObject as slider value (compatible only with integer/float values)
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> ignored </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked (normally, a Layers::LayerMaskAttribute) </param>
			/// <returns> True, when the underlying field modification ends </returns>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const override;
		};
	}

	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::LayerDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::LayerDrawer>();
	template<> void TypeIdDetails::OnRegisterType<Editor::LayerMaskDrawer>();
	template<> void TypeIdDetails::OnUnregisterType<Editor::LayerMaskDrawer>();
}
