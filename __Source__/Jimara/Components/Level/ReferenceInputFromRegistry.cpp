#include "ReferenceInputFromRegistry.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	struct FieldFromRegistry::Helpers {
		static void UpdateLinkedField(FieldFromRegistry* self) {
			self->m_scheduledCounter = 0u;
			Component* parent = self->Parent();
			if (parent == nullptr) 
				return;
			const Reference<Object> value = self->StoredObject();
			if ((!self->m_assignIfNull.load()) && value == nullptr)
				return;
			auto inspect = [&](const Serialization::SerializedObject& obj) {
				const Serialization::ObjectReferenceSerializer* const serializer = obj.As<Serialization::ObjectReferenceSerializer>();
				if (serializer == nullptr)
					return;
				if (serializer->TargetName() != self->FieldName())
					return;
				if ((!self->m_assignIfNull.load()) && (!serializer->ReferencedValueType().CheckType(value)))
					return;
				serializer->SetObjectValue(value, obj.TargetAddr());
			};
			parent->GetFields(Callback<Serialization::SerializedObject>::FromCall(&inspect));
		}

		static void UpdateLinkedFieldLater(FieldFromRegistry* self) {
			if (self->m_scheduledCounter.fetch_add(1u) > 0u)
				return;
			void (*updateLinkedField)(FieldFromRegistry*, Object*) = [](FieldFromRegistry* selfPtr, Object*) {
				if (!selfPtr->Destroyed())
					UpdateLinkedField(selfPtr);
			};
			self->Context()->ExecuteAfterUpdate(Callback<Object*>(updateLinkedField, self), self);
		}

		static void OnReferenceDirty(FieldFromRegistry* self, RegistryReference<Object>*) {
			UpdateLinkedFieldLater(self);
		}

		static void OnParentChanged(FieldFromRegistry* self, ParentChangeInfo) {
			UpdateLinkedFieldLater(self);
		}
	};

	FieldFromRegistry::FieldFromRegistry(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, ReferenceInputFromRegistry<Object>(parent, name) {
		RegistryReference<Object>::OnReferenceDirty() += Callback<RegistryReference<Object>*>(Helpers::OnReferenceDirty, this);
		OnParentChanged() += Callback<ParentChangeInfo>(Helpers::OnParentChanged, this);
		Helpers::UpdateLinkedFieldLater(this);
	}

	FieldFromRegistry::~FieldFromRegistry() {
		RegistryReference<Object>::OnReferenceDirty() -= Callback<RegistryReference<Object>*>(Helpers::OnReferenceDirty, this);
		OnParentChanged() -= Callback<ParentChangeInfo>(Helpers::OnParentChanged, this);
	}

	void FieldFromRegistry::SetFieldName(const std::string_view& name) {
		if (m_fieldName == name)
			return;
		m_fieldName = name;
		Helpers::UpdateLinkedFieldLater(this);
	}

	void FieldFromRegistry::ClearIfNull(bool assign) {
		if (m_assignIfNull.exchange(assign) == assign)
			return;
		Helpers::UpdateLinkedFieldLater(this);
	}

	void FieldFromRegistry::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		RegistryReference<Object>::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(GetFieldName, SetFieldName, "Field Name",
				"Parent Component's Object Reference field of this name will be linked to the registry entry.\n"
				"Keep in mind, that actual linked field changes may be delayed by a frame for some safety reasons.\n"
				"Currently, 'nested' fields are not supported.");
			JIMARA_SERIALIZE_FIELD_GET_SET(ClearIfNull, ClearIfNull, "Clear If Null",
				"If set, this flag will also allow FieldFromRegistry to clear parent fields when the registry has no entry");
		};
	}


	template<> void TypeIdDetails::GetParentTypesOf<ComponentFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<ReferenceInputFromRegistry<Component>>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<ComponentFromRegistry>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<ComponentFromRegistry>(
			"Component From Registry", "Jimara/Level/ComponentFromRegistry", "Component reference input from Registry");
		report(factory);
	}
	template<> void TypeIdDetails::GetParentTypesOf<FieldFromRegistry>(const Callback<TypeId>& report) {
		report(TypeId::Of<ReferenceInputFromRegistry<Object>>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<FieldFromRegistry>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<FieldFromRegistry>(
			"Fields From Registry", "Jimara/Level/FieldFromRegistry", "Object field reference from Registry");
		report(factory);
	}
}
