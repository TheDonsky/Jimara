#include "MaterialShader.h"
#include "../Serialization/Attributes/CustomEditorNameAttribute.h"
#include "../Serialization/DefaultSerializer.h"


namespace Jimara {
namespace Refactor {
	struct Material::Helpers {
		template<typename Type>
		inline static Property MakeProperty(
			const std::string_view& name, const std::string_view& alias, const std::string_view& hint,
			PropertyType type, const Type& defaultValue) {
			static_assert(offsetof(Material::PropertyValue, fp32) == 0u);
			static_assert(offsetof(Material::PropertyValue, fp64) == 0u);
			static_assert(offsetof(Material::PropertyValue, int32) == 0u);
			static_assert(offsetof(Material::PropertyValue, uint32) == 0u);
			static_assert(offsetof(Material::PropertyValue, int64) == 0u);
			static_assert(offsetof(Material::PropertyValue, uint64) == 0u);
			static_assert(offsetof(Material::PropertyValue, bool32) == 0u);
			static_assert(offsetof(Material::PropertyValue, vec2) == 0u);
			static_assert(offsetof(Material::PropertyValue, vec3) == 0u);
			static_assert(offsetof(Material::PropertyValue, vec4) == 0u);
			static_assert(offsetof(Material::PropertyValue, mat4) == 0u);
			Property prop = {};
			prop.name = name;
			prop.alias = alias;
			prop.hint = hint;
			prop.type = type;
			(*reinterpret_cast<Type*>(&prop.defaultValue)) = defaultValue;
			return prop;
		}
	};


	size_t Material::PropertySize(PropertyType type) {
		switch (type) {
		case PropertyType::FLOAT:
			static_assert(sizeof(float) == 4u);
			return sizeof(float);
		case PropertyType::DOUBLE:
			static_assert(sizeof(double) == 8u);
			return sizeof(double);
		case PropertyType::INT32:
			static_assert(sizeof(int32_t) == 4u);
			return sizeof(int32_t);
		case PropertyType::UINT32:
			static_assert(sizeof(uint32_t) == 4u);
			return sizeof(uint32_t);
		case PropertyType::INT64:
			static_assert(sizeof(int64_t) == 8u);
			return sizeof(int64_t);
		case PropertyType::UINT64:
			static_assert(sizeof(uint64_t) == 8u);
			return sizeof(uint64_t);
		case PropertyType::BOOL32:
			static_assert(sizeof(uint32_t) == 4u);
			return sizeof(uint32_t);
		case PropertyType::VEC2:
			static_assert(sizeof(Vector2) == 8u);
			return sizeof(Vector2);
		case PropertyType::VEC3:
			static_assert(sizeof(Vector3) == 12u);
			return sizeof(Vector3);
		case PropertyType::VEC4:
			static_assert(sizeof(Vector4) == 16u);
			return sizeof(Vector4);
		case PropertyType::MAT4:
			static_assert(sizeof(Matrix4) == 64u);
			return sizeof(Matrix4);
		case PropertyType::SAMPLER2D:
			static_assert(sizeof(Vector4) == 16u);
			return sizeof(Vector4);
		default:
			return 0u;
		}
	}

	size_t Material::PropertyAlignment(PropertyType type) {
		switch (type) {
		case PropertyType::FLOAT:
			static_assert(alignof(float) == 4u);
			return alignof (float);
		case PropertyType::DOUBLE:
			static_assert(alignof(double) == 8u);
			return alignof (double);
		case PropertyType::INT32:
			static_assert(alignof (int32_t) == 4u);
			return alignof (int32_t);
		case PropertyType::UINT32:
			static_assert(alignof (uint32_t) == 4u);
			return alignof (uint32_t);
		case PropertyType::INT64:
			static_assert(alignof (int64_t) == 8u);
			return alignof (int64_t);
		case PropertyType::UINT64:
			static_assert(alignof (uint64_t) == 8u);
			return alignof (uint64_t);
		case PropertyType::BOOL32:
			static_assert(alignof (uint32_t) == 4u);
			return alignof (uint32_t);
		case PropertyType::VEC2:
			return 8u;
		case PropertyType::VEC3:
			return 16u;
		case PropertyType::VEC4:
			return 16u;
		case PropertyType::MAT4:
			return 16u;
		case PropertyType::SAMPLER2D:
			return 16u;
		default:
			return 1u;
		}
	}


	Material::Property Material::Property::Float(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, float defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::FLOAT, defaultValue);
	}
	Material::Property Material::Property::Double(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, double defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::DOUBLE, defaultValue);
	}
	Material::Property Material::Property::Int32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int32_t defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::INT32, defaultValue);
	}
	Material::Property Material::Property::Uint32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint32_t defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::UINT32, defaultValue);
	}
	Material::Property Material::Property::Int64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int64_t defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::INT64, defaultValue);
	}
	Material::Property Material::Property::Uint64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint64_t defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::UINT64, defaultValue);
	}
	Material::Property Material::Property::Bool32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, bool defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::BOOL32, defaultValue);
	}
	Material::Property Material::Property::Vec2(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector2 defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::VEC2, defaultValue);
	}
	Material::Property Material::Property::Vec3(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector3 defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::VEC3, defaultValue);
	}
	Material::Property Material::Property::Vec4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::VEC4, defaultValue);
	}
	Material::Property Material::Property::Mat4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Matrix4 defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::MAT4, defaultValue);
	}
	Material::Property Material::Property::Sampler2D(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue) {
		return Helpers::MakeProperty(name, hint, alias, PropertyType::SAMPLER2D, defaultValue);
	}


	Material::LitShader::LitShader(const OS::Path& litShaderPath, const std::vector<Material::Property>& properties) {
		m_shaderPath = litShaderPath;
		m_propertyBufferSize = 0u;
		m_propertyBufferAlignment = 1u;
		size_t samplerCount = 0u;
		for (size_t i = 0u; i < properties.size(); i++) {
			const Material::Property& prop = properties[i];
			PropertyInfo& info = m_properties.emplace_back();
			info.name = prop.name;
			info.alias = prop.alias;
			info.hint = prop.hint;
			info.type = prop.type;
			info.defaultValue = prop.defaultValue;
			if (info.type == PropertyType::SAMPLER2D) {
				std::stringstream stream;
				stream << "jm_MaterialSamplerBinding" << samplerCount;
				samplerCount++;
				info.bindingName = stream.str();
			}
			else info.bindingName = SETTINGS_BUFFER_BINDING_NAME;

			const size_t alignment = PropertyAlignment(info.type);
			m_propertyBufferAlignment = Math::Max(m_propertyBufferAlignment, alignment);

			info.settingsBufferOffset = ((m_propertyBufferSize + alignment - 1u) / alignment) * alignment;
			m_propertyBufferSize = info.settingsBufferOffset + PropertySize(info.type);

			info.serializer = [&]() -> Reference<const Serialization::ItemSerializer> {
				std::vector<Reference<const Object>> attributeList = { 
					Object::Instantiate<Serialization::CustomEditorNameAttribute>((info.alias.length() > 0u) ? info.alias : info.name)
				};
				switch (info.type) {
				case PropertyType::FLOAT:
					return Serialization::ValueSerializer<float>::Create(info.name, info.hint, attributeList);
				case PropertyType::DOUBLE:
					return Serialization::ValueSerializer<double>::Create(info.name, info.hint, attributeList);
				case PropertyType::INT32:
					return Serialization::ValueSerializer<int32_t>::Create(info.name, info.hint, attributeList);
				case PropertyType::UINT32:
					return Serialization::ValueSerializer<uint32_t>::Create(info.name, info.hint, attributeList);
				case PropertyType::INT64:
					return Serialization::ValueSerializer<int64_t>::Create(info.name, info.hint, attributeList);
				case PropertyType::UINT64:
					return Serialization::ValueSerializer<uint64_t>::Create(info.name, info.hint, attributeList);
				case PropertyType::BOOL32:
					return Serialization::ValueSerializer<bool>::For<uint32_t>(info.name, info.hint,
						Function<bool, uint32_t*>([](uint32_t* v) -> bool { return *v; }),
						Callback<const bool&, uint32_t*>([](const bool& b, uint32_t* v) { (*v) = b; }),
						attributeList);
				case PropertyType::VEC2:
					Serialization::ValueSerializer<Vector2>::Create(info.name, info.hint, attributeList);
				case PropertyType::VEC3:
					Serialization::ValueSerializer<Vector3>::Create(info.name, info.hint, attributeList);
				case PropertyType::VEC4:
					Serialization::ValueSerializer<Vector4>::Create(info.name, info.hint, attributeList);
				case PropertyType::MAT4:
					Serialization::ValueSerializer<Matrix4>::Create(info.name, info.hint, attributeList);
				case PropertyType::SAMPLER2D:
					Serialization::DefaultSerializer<Reference<Graphics::TextureSampler>>::Create(info.name, info.hint, attributeList);
				default:
					return nullptr;
				}
			}();
		}

		for (size_t i = 0u; i < m_properties.size(); i++) {
			const PropertyInfo& info = m_properties[i];
			m_propertyIdByName[info.name] = i;
			m_propertyIdByBindingName[info.name] = i;
		}
	}

	size_t Material::LitShader::PropertyIdByName(const std::string_view& name)const {
		const auto it = m_propertyIdByName.find(name);
		if (it != m_propertyIdByName.end())
			return it->second;
		else return NO_ID;
	}

	size_t Material::LitShader::PropertyIdByBindingName(const std::string_view& bindingName)const {
		const auto it = m_propertyIdByBindingName.find(bindingName);
		if (it != m_propertyIdByBindingName.end())
			return it->second;
		else return NO_ID;
	}
}
}
