#pragma once
#include "../ImGuiRenderer.h"
#include <Jimara/Data/Serialization/Helpers/SerializerTypeMask.h>
#include <Jimara/Data/Serialization/Attributes/CustomEditorNameAttribute.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Draws arbitrary SerializedObject with ImGui fields
		/// </summary>
		/// <param name="object"> SerializedObject </param>
		/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <param name="drawObjectPtrSerializedObject"> This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one </param>
		/// <returns> True, if any underlying field modification ends </returns>
		bool DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject);

		/// <summary>
		/// Draws arbitrary SerializedObject with ImGui fields
		/// </summary>
		/// <typeparam name="DrawObjectPtrSerializedObjectCallback"> Anything that can be invoked with constant Serialization::SerializedObject reference as an argument </typeparam>
		/// <param name="object"> SerializedObject </param>
		/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <param name="drawObjectPtrSerializedObject"> This function has no idea how to display OBJECT_PTR_VALUE types and invokes this callback each time it encounters one </param>
		/// <returns> True, if any underlying field modification ends </returns>
		template<typename DrawObjectPtrSerializedObjectCallback>
		bool DrawSerializedObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const DrawObjectPtrSerializedObjectCallback& drawObjectPtrSerializedObject) {
			return DrawSerializedObject(object, viewId, logger, Function<bool, const Serialization::SerializedObject&>::FromCall(&drawObjectPtrSerializedObject));
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
			/// <returns> True, if any underlying field modification ends </returns>
			virtual bool DrawObject(
				const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
				const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const = 0;

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

			/// <summary>
			/// Default id for a serialized object field
			/// </summary>
			/// <param name="object"> Target object </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <returns> 
			/// "{object.Serializer()->TargetName()}###DrawSerializedObject_for_view_{viewId}_serializer_{object.Serializer()}_target_{object.TargetAddr()}" 
			/// </returns>
			inline static std::string DefaultGuiItemName(const Serialization::SerializedObject& object, size_t viewId) {
				std::stringstream stream;
				const Serialization::CustomEditorNameAttribute* customName =
					object.Serializer()->FindAttributeOfType<Serialization::CustomEditorNameAttribute>();
				stream << ((customName == nullptr) ? object.Serializer()->TargetName() : customName->CustomName())
					<< "###DrawSerializedObject_for_view_" << viewId
					<< "_serializer_" << ((size_t)(object.Serializer()))
					<< "_target_" << ((size_t)(object.TargetAddr()));
				return stream.str();
			}
		};


		/// <summary>
		/// Depending on what attributes each ItemSerializer has, DrawSerializedObject may be required to add some custom stuff to the default serializers.
		/// SerializedObjectDecoratorDrawer enables custom behaviour.
		/// <para/> Note: In order to make a SerializedObjectDecoratorDrawer active and available, 
		///		some registered class has to return an instance through type attributes.
		/// </summary>
		class SerializedObjectDecoratorDrawer : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="attributeType"> Type of an attribute that triggers this drawer </param>
			inline SerializedObjectDecoratorDrawer(const TypeId& attributeType) : m_attributeTypeId(attributeType) {}

			/// <summary> Type of an attribute that triggers this drawer </summary>
			inline TypeId AttributeType()const { return m_attributeTypeId; }

			/// <summary>
			/// Should draw a custom decorator for a SerializedObject
			/// </summary>
			/// <param name="object"> SerializedObject </param>
			/// <param name="viewId"> Unique identifier for the ImGui window/view (calling context) </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="attribute"> Attribute, that caused this function to be invoked </param>
			/// <returns> True, if any underlying field modification ends </returns>
			virtual bool DecorateObject(const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger, const Object* attribute)const = 0;

		private:
			// Type of an attribute that triggers this drawer
			const TypeId m_attributeTypeId;
		};
	}

	// Parent types of CustomSerializedObjectDrawer
	template<> inline void TypeIdDetails::GetParentTypesOf<Editor::CustomSerializedObjectDrawer>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}
