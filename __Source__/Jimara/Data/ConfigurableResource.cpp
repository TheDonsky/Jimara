#include "ConfigurableResource.h"
#include "Serialization/Attributes/InlineSerializerListAttribute.h"


namespace Jimara {
	ConfigurableResource::InstanceSerializer::InstanceSerializer(
		const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes)
		: Serialization::ItemSerializer(name, hint, attributes) {}

	ConfigurableResource::InstanceSerializer::~InstanceSerializer() {}

	void ConfigurableResource::InstanceSerializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableInstance* target)const {
		// Copy data from old resource to the new one, if possible:
		auto transferOldData = [&](ConfigurableResource* src, ConfigurableResource* dst) {
			if (src == nullptr || dst == nullptr)
				return;
			// __TODO__: [Maybe] Transfer data from the old resource...
		};

		// Recreate resource:
		auto recreateResource = [&](const ResourceFactory* factory) {
			if (factory == nullptr) {
				target->instance = nullptr;
				return;
			}
			const Reference<ConfigurableResource> oldResource = target->instance;
			try {
				target->instance = factory->CreateInstance(target->recreateArgs);
			}
			catch (std::exception&) {
				target->instance = nullptr;
				return;
			}
			transferOldData(oldResource, target->instance);
		};

		// Swap-out instance if needed:
		{
			const Reference<const ResourceFactory::Set> factories = ResourceFactory::All();
			const Reference<const ResourceFactory> oldFactory = target->instance == nullptr ? nullptr : factories->FindFactory(target->instance.operator->());
			Reference<const ResourceFactory> newFactory = oldFactory;
			static const ResourceFactory::RegisteredInstanceSerializer factorySerializer("Resource Type", 
				"Configurable Resource Type (Keep in mind, that changing this overrides existing resource)");
			factorySerializer.GetFields(recordElement, &newFactory);
			if (oldFactory != newFactory)
				recreateResource(newFactory);
		}

		// Serialize the instance:
		if (target->instance != nullptr) {
			static const ConfigurableResource::Serializer serializer("Resource Data", "Configurable Resource Data",
				std::vector<Reference<const Object>>{ Serialization::InlineSerializerListAttribute::Instance() });
			recordElement(serializer.Serialize(target->instance));
		}
	}

	const ConfigurableResource::InstanceSerializer* ConfigurableResource::InstanceSerializer::Instance() {
		static const InstanceSerializer instance("Configurable Resource Instance", "Configurable Resource Instance serializer instance");
		return &instance;
	}
}
