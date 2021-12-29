#pragma once
#include "../../Environment/JimaraEditor.h"
#include "../../GUI/ImGuiRenderer.h"
#include <Data/Serialization/Helpers/SerializerTypeMask.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Draws arbitrary SerializedObject with ImGui fields
		/// </summary>
		/// <param name="object"> SerializedObject </param>
		/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <param name="drawObjectPtrSerializedObject"> This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one </param>
		void DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Callback<const Serialization::SerializedObject&>& drawObjectPtrSerializedObject);

		/// <summary>
		/// Draws arbitrary SerializedObject with ImGui fields
		/// </summary>
		/// <typeparam name="DrawObjectPtrSerializedObjectCallback"> Anything that can be invoked with constant Serialization::SerializedObject reference as an argument </typeparam>
		/// <param name="object"> SerializedObject </param>
		/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <param name="drawObjectPtrSerializedObject"> This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one </param>
		template<typename DrawObjectPtrSerializedObjectCallback>
		void DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const DrawObjectPtrSerializedObjectCallback& drawObjectPtrSerializedObject) {
			void(*callback)(const DrawObjectPtrSerializedObjectCallback*, const Serialization::SerializedObject&) =
				[](const DrawObjectPtrSerializedObjectCallback* call, const Serialization::SerializedObject& object) { (*call)(object); };
			DrawSerializedObject(object, viewId, logger, Callback<const Serialization::SerializedObject&>(callback, &drawObjectPtrSerializedObject));
		}

		/// <summary>
		/// Depending on what attributes each ItemSerializer has, DrawSerializedObject may be required to draw known types differently;
		/// This interface is one to implement to define such behaviour
		/// </summary>
		class CustomSerializedObjectDrawer : public virtual Object {
		public:
			/// <summary>
			/// Should draw a SerializedObject in some custom way
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="drawObjectPtrSerializedObject"> 
			///		This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one 
			/// </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked </param>
			virtual void DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Callback<const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const = 0;

			/// <summary>
			/// Registers CustomSerializedObjectDrawer per given serializer types for some attribute type
			/// Note: Register() and Unregister() calls should be invoked one for one, matching serializerTypes and serializerAttributeType for worry-free usage
			/// </summary>
			/// <param name="serializerTypes"> Types of serializers this drawer can operate on </param>
			/// <param name="serializerAttributeType"> Type of attributes that will trigger custom drawers </param>
			void Register(Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType)const;

			/// <summary>
			/// Unregisters CustomSerializedObjectDrawer per given serializer types for some attribute type
			/// Note: Register() and Unregister() calls should be invoked one for one, matching serializerTypes and serializerAttributeType for worry-free usage
			/// </summary>
			/// <param name="serializerTypes"> Types of serializers this drawer could operate on </param>
			/// <param name="serializerAttributeType"> Type of attributes that used to trigger custom drawers </param>
			void Unregister(Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType)const;
		};
	}

	// Parent types of CustomSerializedObjectDrawer
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::CustomSerializedObjectDrawer>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}
