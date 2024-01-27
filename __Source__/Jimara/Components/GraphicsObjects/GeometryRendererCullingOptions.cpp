#include "GeometryRendererCullingOptions.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/CustomEditorNameAttribute.h"


namespace Jimara {
	bool GeometryRendererCullingOptions::operator==(const GeometryRendererCullingOptions& other)const {
		return
			boundaryThickness == other.boundaryThickness &&
			boundaryOffset == other.boundaryOffset &&
			onScreenSizeRangeStart == other.onScreenSizeRangeStart &&
			onScreenSizeRangeEnd == other.onScreenSizeRangeEnd;
	}



	void GeometryRendererCullingOptions::Serializer::GetFields(
		const Callback<Serialization::SerializedObject>& recordElement, GeometryRendererCullingOptions* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			JIMARA_SERIALIZE_FIELD(target->boundaryThickness, "Boundary Thickness",
				"'Natural' culling boundary of the geometry will be expanded by this amount in each direction in local space\n"
				"(Useful for the cases when the shader does some vertex displacement and the visible geometry goes out of initial boundaries)");
			JIMARA_SERIALIZE_FIELD(target->boundaryOffset, "Boundary Offset",
				"Local-space culling boundary will be offset by this amount");

			static const constexpr std::string_view onScreenSizeRangeHint =
				"Object will be visible if and only if the object occupies \n"
				"a fraction of the viewport between Min and Max on-screen sizes; \n"
				"If Max On-Screen Size is negative, it will be interpreted as unbounded \n"
				"(Hint: You can buld LOD systems with these)";
			JIMARA_SERIALIZE_FIELD(target->onScreenSizeRangeStart, "Min On-Screen Size", onScreenSizeRangeHint);
			{
				const bool onScreenSizeWasPresent = (target->onScreenSizeRangeEnd >= 0.0f);
				bool hasMaxOnScreenSize = onScreenSizeWasPresent;
				JIMARA_SERIALIZE_FIELD(hasMaxOnScreenSize, "Has Max On-Screen Size", onScreenSizeRangeHint);
				if (hasMaxOnScreenSize != onScreenSizeWasPresent) {
					if (hasMaxOnScreenSize)
						target->onScreenSizeRangeEnd = Math::Max(1.0f, target->onScreenSizeRangeEnd);
					else target->onScreenSizeRangeEnd = -1.0f;
				}
			}
			if (target->onScreenSizeRangeEnd >= 0.0f)
				JIMARA_SERIALIZE_FIELD(target->onScreenSizeRangeEnd, "Max On-Screen Size", onScreenSizeRangeHint);
		};
	}



	GeometryRendererCullingOptions GeometryRendererCullingOptions::Provider::Options()const {
		std::unique_lock<SpinLock>(m_lock);
		GeometryRendererCullingOptions rv = m_options;
		return rv;
	}

	void GeometryRendererCullingOptions::Provider::SetOptions(const GeometryRendererCullingOptions& options) {
		{
			std::unique_lock<SpinLock>(m_lock);
			if (m_options == options)
				return;
			m_options = options;
		}
		m_onDirty(this);
	}

	void GeometryRendererCullingOptions::Provider::GetFields(const Callback<Serialization::SerializedObject> recordElement) {
		static const GeometryRendererCullingOptions::Serializer serializer("Options", "Culling options");
		GeometryRendererCullingOptions options = Options();
		serializer.GetFields(recordElement, &options);
		SetOptions(options);
	}



	GeometryRendererCullingOptions::ConfigurableOptions::ConfigurableOptions(const ConfigurableOptions& other) {
		(*this) = other;
	}

	GeometryRendererCullingOptions::ConfigurableOptions::~ConfigurableOptions() {
		SetConfigurationProvider(nullptr);
	}

	GeometryRendererCullingOptions::ConfigurableOptions& GeometryRendererCullingOptions::ConfigurableOptions::operator=(const ConfigurableOptions& other) {
		if (this == (&other))
			return (*this);
		const Reference<Provider> provider = other.ConfigurationProvider();
		SetConfigurationProvider(provider);
		m_configuration = other.m_configuration;
		return (*this);
	}

	GeometryRendererCullingOptions::ConfigurableOptions& GeometryRendererCullingOptions::ConfigurableOptions::operator=(const GeometryRendererCullingOptions& options) {
		if (m_configuration == options)
			return (*this);
		m_configuration = options;
		m_onDirty(this);
		return (*this);
	}

	Reference<GeometryRendererCullingOptions::Provider> GeometryRendererCullingOptions::ConfigurableOptions::ConfigurationProvider()const {
		std::unique_lock<SpinLock> lock(m_providerLock);
		Reference<Provider> rv = m_configurationProvider;
		return rv;
	}

	void GeometryRendererCullingOptions::ConfigurableOptions::SetConfigurationProvider(Provider* provider) {
		{
			std::unique_lock<SpinLock> lock(m_providerLock);
			if (m_configurationProvider != provider) {
				typedef void(*OnConfigurationDirty)(ConfigurableOptions*, const Provider*);
				static const OnConfigurationDirty onConfigurationDirty = [](ConfigurableOptions* self, const Provider* prov) {
					assert(self != nullptr);
					assert(prov != nullptr);
					(*self) = prov->Options();
				};
				const Callback<const Provider*> onDirty(onConfigurationDirty, this);
				if (m_configurationProvider != nullptr)
					m_configurationProvider->OnOptionsChanged() -= onDirty;
				m_configurationProvider = provider;
				if (m_configurationProvider != nullptr)
					m_configurationProvider->OnOptionsChanged() += onDirty;
			}
			if (m_configurationProvider != nullptr)
				m_configuration = m_configurationProvider->Options();
		}
		m_onDirty(this);
	}

	GeometryRendererCullingOptions::ConfigurableOptions::Serializer::Serializer(
		const std::string_view& name, const std::string_view& hint, const std::vector<Reference<const Object>>& attributes) 
		: Serialization::ItemSerializer(name, hint, [&]() -> std::vector<Reference<const Object>> {
		std::vector<Reference<const Object>> rv;
		bool foundInlineAttribute = false;
		for (size_t i = 0u; i < attributes.size(); i++) {
			const Object* obj = attributes[i];
			rv.push_back(obj);
			if (dynamic_cast<const Serialization::InlineSerializerListAttribute*>(obj) != nullptr)
				foundInlineAttribute = true;
		}
		if (foundInlineAttribute)
			return rv;
		typedef bool(*CheckFn)(Serialization::SerializedObject);
		static const CheckFn checkIfProviderIsPresent = [](Serialization::SerializedObject object) -> bool {
			if (dynamic_cast<const Serializer*>(object.Serializer()) == nullptr)
				return true;
			ConfigurableOptions* target = (ConfigurableOptions*)(object.TargetAddr());
			if (target == nullptr)
				return true;
			return target->ConfigurationProvider() != nullptr;
			};
		static const Reference<const Object> attribute = Object::Instantiate<Serialization::InlineSerializerListAttribute>(Function(checkIfProviderIsPresent));
		rv.push_back(attribute);
		return rv;
			}())
		, m_inlineSerializer(Serialization::DefaultSerializer<Reference<Provider>>::Create(
			"Configuration Provider", hint, [&]() -> std::vector<Reference<const Object>> {
				std::vector<Reference<const Object>> rv;
				bool founCustomNameAttribute = false;
				for (size_t i = 0u; i < attributes.size(); i++) {
					const Object* obj = attributes[i];
					rv.push_back(obj);
					if (dynamic_cast<const Serialization::CustomEditorNameAttribute*>(obj) != nullptr)
						founCustomNameAttribute = true;
				}
				if (!founCustomNameAttribute)
					rv.push_back(Object::Instantiate<Serialization::CustomEditorNameAttribute>(name));
				return rv;
			}())) {}

	void GeometryRendererCullingOptions::ConfigurableOptions::Serializer::GetFields(
		const Callback<Serialization::SerializedObject>& recordElement, ConfigurableOptions* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			{
				Reference<Provider> configurationProvider = target->ConfigurationProvider();
				if (FindAttributeOfType<Serialization::InlineSerializerListAttribute>()->Check(Serialize(target)))
					recordElement(dynamic_cast<const Serialization::DefaultSerializer<Reference<Provider>>::Serializer_T*>(m_inlineSerializer.operator->())
						->Serialize(configurationProvider));
				else JIMARA_SERIALIZE_FIELD(configurationProvider,
					"Configuration Provider", "Configuration provider for culling options (overrides existing configuration)");
				target->SetConfigurationProvider(configurationProvider);
			}
			if (target->ConfigurationProvider() == nullptr) {
				static const GeometryRendererCullingOptions::Serializer serializer("Options", "Culling Options");
				GeometryRendererCullingOptions options = target->m_configuration;
				serializer.GetFields(recordElement, &options);
				(*target) = options;
			}
		};
	}



	template<> void TypeIdDetails::GetTypeAttributesOf<GeometryRendererCullingOptions::Provider>(const Callback<const Object*>& report) {
		static const Reference<ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<GeometryRendererCullingOptions::Provider>(
			"Geometry Culling Options", "Jimara/Renderers/Geometry Culling Options", "Culling options for geometry renderers");
		report(factory);
	}
}
