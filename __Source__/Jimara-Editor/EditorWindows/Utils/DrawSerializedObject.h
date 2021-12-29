#pragma once
#include "../../Environment/JimaraEditor.h"
#include "../../GUI/ImGuiRenderer.h"
#include <Data/Serialization/Helpers/SerializerTypeMask.h>


namespace Jimara {
	namespace Editor {
		void DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Callback<const Serialization::SerializedObject&>& drawObjectPtrSerializedObject);

		template<typename DrawObjectPtrSerializedObjectCallback>
		void DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const DrawObjectPtrSerializedObjectCallback& drawObjectPtrSerializedObject) {
			void(*callback)(const DrawObjectPtrSerializedObjectCallback*, const Serialization::SerializedObject&) =
				[](const DrawObjectPtrSerializedObjectCallback* call, const Serialization::SerializedObject& object) { (*call)(object); };
			DrawSerializedObject(object, viewId, logger, Callback<const Serialization::SerializedObject&>(callback, &drawObjectPtrSerializedObject));
		}

		typedef Callback<const Serialization::SerializedObject&, size_t, OS::Logger*, const Callback<const Serialization::SerializedObject&>&> CustomSerializedObjectDrawer;
		void RegisterCustomSerializedObjectDrawer(const CustomSerializedObjectDrawer& drawer, Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType);
		void UnregisterCustomSerializedObjectDrawer(const CustomSerializedObjectDrawer& drawer, Serialization::SerializerTypeMask serializerTypes, TypeId serializerAttributeType);
	}
}
