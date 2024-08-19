#include "MaterialShader.h"
#include "../Serialization/Attributes/CustomEditorNameAttribute.h"
#include "../Serialization/Attributes/EnumAttribute.h"
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


	Material::Material(Graphics::GraphicsDevice* graphicsDevice,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers)
		: m_graphicsDevice(graphicsDevice)
		, m_bindlessBuffers(bindlessBuffers)
		, m_bindlessSamplers(bindlessSamplers)
		, m_oneTimeCommandBuffers(Graphics::OneTimeCommandPool::GetFor(graphicsDevice)) {
		assert(graphicsDevice != nullptr);
		if (m_bindlessBuffers == nullptr)
			graphicsDevice->Log()->Fatal("Material::Material - bindlessBuffers not provided! [File: ", __FILE__, "; ", __LINE__, "]");
		if (m_bindlessSamplers == nullptr)
			graphicsDevice->Log()->Fatal("Material::Material - bindlessSamplers not provided! [File: ", __FILE__, "; ", __LINE__, "]");
		if (m_oneTimeCommandBuffers == nullptr)
			graphicsDevice->Log()->Fatal("Material::Material - OneTimeCommandPool could not be obtained! [File: ", __FILE__, "; ", __LINE__, "]");
		Writer(this).SetShader(nullptr);
	}

	Material::~Material() {}

	void Material::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Writer writer(this);
		writer.GetFields(recordElement);
	}


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
		case PropertyType::PAD32:
			static_assert(sizeof(uint32_t) == 4u);
			return sizeof(uint32_t);
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
		case PropertyType::PAD32:
			return 4u;
		default:
			return 1u;
		}
	}

	TypeId Material::PropertyTypeId(PropertyType type) {
		switch (type) {
		case PropertyType::FLOAT:
			return TypeId::Of<float>();
		case PropertyType::DOUBLE:
			return TypeId::Of<double>();
		case PropertyType::INT32:
			return TypeId::Of<int32_t>();
		case PropertyType::UINT32:
			return TypeId::Of<uint32_t>();
		case PropertyType::INT64:
			return TypeId::Of<int64_t>();
		case PropertyType::UINT64:
			return TypeId::Of<uint64_t>();
		case PropertyType::BOOL32:
			return TypeId::Of<bool>();
		case PropertyType::VEC2:
			return TypeId::Of<Vector2>();
		case PropertyType::VEC3:
			return TypeId::Of<Vector3>();
		case PropertyType::VEC4:
			return TypeId::Of<Vector4>();
		case PropertyType::MAT4:
			return TypeId::Of<Matrix4>();
		case PropertyType::SAMPLER2D:
			return TypeId::Of<Graphics::TextureSampler*>();
		default:
			return TypeId::Of<void>();
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
		m_pathStr = m_shaderPath.string();
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
					return Serialization::ValueSerializer<bool>::Create(info.name, info.hint, attributeList);
				case PropertyType::VEC2:
					return Serialization::ValueSerializer<Vector2>::Create(info.name, info.hint, attributeList);
				case PropertyType::VEC3:
					return Serialization::ValueSerializer<Vector3>::Create(info.name, info.hint, attributeList);
				case PropertyType::VEC4:
					return Serialization::ValueSerializer<Vector4>::Create(info.name, info.hint, attributeList);
				case PropertyType::MAT4:
					return Serialization::ValueSerializer<Matrix4>::Create(info.name, info.hint, attributeList);
				case PropertyType::SAMPLER2D:
					return Serialization::DefaultSerializer<Reference<Graphics::TextureSampler>>::Create(info.name, info.hint, attributeList);
				default:
					return nullptr;
				}
			}();
		}

		for (size_t i = 0u; i < m_properties.size(); i++) {
			const PropertyInfo& info = m_properties[i];
			if (info.name.length() <= 0)
				continue;
			m_propertyIdByName[info.name] = i;
			if (info.type == PropertyType::SAMPLER2D)
				m_propertyIdByBindingName[info.bindingName] = i;
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


	Reference<Material::LitShaderSet> Material::LitShaderSet::All() {
		static SpinLock allLock;
		static Reference<LitShaderSet> all;
		static Reference<RegisteredTypeSet> registeredTypes;

		std::unique_lock<SpinLock> lock(allLock);
		{
			const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
			if (currentTypes == registeredTypes) return all;
			else registeredTypes = currentTypes;
		}
		std::set<Reference<const LitShader>> shaders;
		for (size_t i = 0; i < registeredTypes->Size(); i++) {
			void(*checkAttribute)(decltype(shaders)*, const Object*) = [](decltype(shaders)* shaderSet, const Object* attribute) {
				const LitShader* litShader = dynamic_cast<const LitShader*>(attribute);
				if (litShader != nullptr)
					shaderSet->insert(litShader);
				};
			registeredTypes->At(i).GetAttributes(Callback<const Object*>(checkAttribute, &shaders));
		}
		all = new LitShaderSet(shaders);
		all->ReleaseRef();
		return all;
	}

	size_t Material::LitShaderSet::Size()const { return m_shaders.size(); }

	const Material::LitShader* Material::LitShaderSet::At(size_t index)const { return m_shaders[index]; }

	const Material::LitShader* Material::LitShaderSet::operator[](size_t index)const { return At(index); }

	const Material::LitShader* Material::LitShaderSet::FindByPath(const OS::Path& shaderPath)const {
		const decltype(m_shadersByPath)::const_iterator it = m_shadersByPath.find(shaderPath);
		if (it == m_shadersByPath.end()) return nullptr;
		else return it->second;
	}

	size_t Material::LitShaderSet::IndexOf(const LitShader* litShader)const {
		IndexPerShader::const_iterator it = m_indexPerShader.find(litShader);
		if (it == m_indexPerShader.end()) return NO_ID;
		else return it->second;
	}

	Material::LitShaderSet::LitShaderSet(const std::set<Reference<const LitShader>>& shaders)
		: m_shaders(shaders.begin(), shaders.end())
		, m_indexPerShader([&]() {
		IndexPerShader index;
		for (std::set<Reference<const LitShader>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
			index.insert(std::make_pair(it->operator->(), index.size()));
		return index;
			}())
		, m_shadersByPath([&]() {
		ShadersByPath shadersByPath;
		for (std::set<Reference<const LitShader>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
			const std::string path = it->operator->()->LitShaderPath();
			if (path.size() > 0)
				shadersByPath[path] = *it;
		}
		return shadersByPath;
			}())
		, m_classSelector([&]() {
		typedef Serialization::EnumAttribute<std::string_view> EnumAttribute;
		std::vector<EnumAttribute::Choice> choices;
		choices.push_back(EnumAttribute::Choice("<None>", ""));
		for (std::set<Reference<const LitShader>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
			const std::string& path = it->operator->()->m_pathStr;
			choices.push_back(EnumAttribute::Choice(path, path));
		}
		typedef std::string_view(*GetFn)(const LitShader**);
		typedef void(*SetFn)(const std::string_view&, const LitShader**);
		return Serialization::ValueSerializer<std::string_view>::For<const LitShader*>("Shader", "Lit Shader",
			(GetFn)[](const LitShader** litShader)->std::string_view {
			if (litShader == nullptr || (*litShader) == nullptr) return "";
			else return (*litShader)->m_pathStr;
		},
			(SetFn)[](const std::string_view& value, const LitShader** litShader) {
			if (litShader == nullptr) return;
			Reference<LitShaderSet> allShaders = LitShaderSet::All();
			(*litShader) = allShaders->FindByPath(value);
		}, { Object::Instantiate<EnumAttribute>(choices, false) });
			}()) {
#ifndef NDEBUG
		for (size_t i = 0; i < m_shaders.size(); i++)
			assert(IndexOf(m_shaders[i]) == i);
#endif // !NDEBUG
	}



	Material::Writer::Writer(Material* material)
		: m_material(material), m_flags(0u) {
		if (m_material != nullptr)
			m_material->m_readWriteLock.lock();
	}

	Material::Writer::~Writer() {
		if (m_material == nullptr)
			return;

		if ((m_flags & FLAG_FIELDS_DIRTY) != 0) {
			
			m_material->m_settingsConstantBuffer = m_material->GraphicsDevice()->CreateConstantBuffer(m_material->m_settingsBufferData.Size());
			assert(m_material->m_settingsConstantBuffer != nullptr);
			std::memcpy(m_material->m_settingsConstantBuffer->Map(), (const void*)m_material->m_settingsBufferData.Data(), m_material->m_settingsBufferData.Size());
			m_material->m_settingsConstantBuffer->Unmap(true);

			Reference<Graphics::ArrayBuffer> scratchBuffer =
				m_material->GraphicsDevice()->CreateArrayBuffer(m_material->m_settingsBufferData.Size(), 1, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
			assert(scratchBuffer != nullptr);
			std::memcpy(scratchBuffer->Map(), (const void*)m_material->m_settingsBufferData.Data(), m_material->m_settingsBufferData.Size());
			scratchBuffer->Unmap(true);

			Reference<Graphics::ArrayBuffer> bindlessBuffer =
				m_material->GraphicsDevice()->CreateArrayBuffer(m_material->m_settingsBufferData.Size(), 1, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
			assert(bindlessBuffer != nullptr);
			{
				Graphics::OneTimeCommandPool::Buffer commandBuffer(m_material->m_oneTimeCommandBuffers);
				bindlessBuffer->Copy(commandBuffer, scratchBuffer);
				commandBuffer.SubmitAsynch();
			}

			m_material->m_settingsBufferId = m_material->BindlessBuffers()->GetBinding(bindlessBuffer);
			assert(m_material->m_settingsBufferId != nullptr);
		}

		m_material->m_readWriteLock.unlock();
		if ((m_flags & FLAG_FIELDS_DIRTY) != 0)
			m_material->m_onMaterialDirty(m_material);
		if ((m_flags & FLAG_SHADER_DIRTY) != 0)
			m_material->m_onInvalidateSharedInstance(m_material);
	}

	void Material::Writer::SetShader(const LitShader* shader) {
		if (m_material == nullptr || (shader != nullptr && m_material->m_shader == shader))
			return;

		std::unordered_map<std::string_view, PropertyValue> propertyValues;
		std::unordered_map<std::string_view, Reference<Graphics::TextureSampler>> samplerValues;

		auto cachePropertyValue = [&](const PropertyInfo& info) {
			switch (info.type) {
			case PropertyType::FLOAT:
				propertyValues[info.name].fp32 = GetPropertyValue<float>(info.name);
				break;
			case PropertyType::DOUBLE:
				propertyValues[info.name].fp64 = GetPropertyValue<double>(info.name);
				break;
			case PropertyType::INT32:
				propertyValues[info.name].int32 = GetPropertyValue<int32_t>(info.name);
				break;
			case PropertyType::UINT32:
				propertyValues[info.name].uint32 = GetPropertyValue<uint32_t>(info.name);
				break;
			case PropertyType::INT64:
				propertyValues[info.name].int64 = GetPropertyValue<int64_t>(info.name);
				break;
			case PropertyType::UINT64:
				propertyValues[info.name].uint64 = GetPropertyValue<uint64_t>(info.name);
				break;
			case PropertyType::BOOL32:
				propertyValues[info.name].bool32 = GetPropertyValue<bool>(info.name);
				break;
			case PropertyType::VEC2:
				propertyValues[info.name].vec2 = GetPropertyValue<Vector2>(info.name);
				break;
			case PropertyType::VEC3:
				propertyValues[info.name].vec3 = GetPropertyValue<Vector3>(info.name);
				break;
			case PropertyType::VEC4:
				propertyValues[info.name].vec4 = GetPropertyValue<Vector4>(info.name);
				break;
			case PropertyType::MAT4:
				propertyValues[info.name].mat4 = GetPropertyValue<Matrix4>(info.name);
				break;
			case PropertyType::SAMPLER2D:
				samplerValues[info.name] = GetPropertyValue<Graphics::TextureSampler*>(info.name);
				break;
			}
		};

		const Reference<const LitShader> oldShader = m_material->m_shader;
		if (oldShader != nullptr)
			for (size_t i = 0u; i < oldShader->PropertyCount(); i++)
				cachePropertyValue(oldShader->Property(i));

		m_material->m_shader = shader;
		m_material->m_imageByBindingName.clear();
		auto restorePropertyValue = [&](const PropertyInfo& info) {
			static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(info)>>, PropertyInfo>);
			auto restoreValue = [&](auto value) {
				using ValueType = std::remove_const_t<std::remove_reference_t<decltype(value)>>;
				size_t oldId = (oldShader == nullptr) ? NO_ID : oldShader->PropertyIdByName(info.name);
				if (oldId != NO_ID && oldShader->Property(oldId).type == info.type) {
					auto it = propertyValues.find(info.name);
					assert(it != propertyValues.end());
					value = *((ValueType*)((void*)(&it->second)));
				}
				SetPropertyValue<ValueType>(info.name, value);
			};
			switch (info.type) {
			case PropertyType::FLOAT:
				restoreValue(info.defaultValue.fp32);
				break;
			case PropertyType::DOUBLE:
				restoreValue(info.defaultValue.fp64);
				break;
			case PropertyType::INT32:
				restoreValue(info.defaultValue.int32);
				break;
			case PropertyType::UINT32:
				restoreValue(info.defaultValue.uint32);
				break;
			case PropertyType::INT64:
				restoreValue(info.defaultValue.int64);
				break;
			case PropertyType::UINT64:
				restoreValue(info.defaultValue.uint64);
				break;
			case PropertyType::BOOL32:
				restoreValue(info.defaultValue.bool32);
				break;
			case PropertyType::VEC2:
				restoreValue(info.defaultValue.vec2);
				break;
			case PropertyType::VEC3:
				restoreValue(info.defaultValue.vec3);
				break;
			case PropertyType::VEC4:
				restoreValue(info.defaultValue.vec4);
				break;
			case PropertyType::MAT4:
				restoreValue(info.defaultValue.mat4);
				break;
			case PropertyType::SAMPLER2D:
			{
				auto it = samplerValues.find(info.name);
				if (it != samplerValues.end())
					SetPropertyValue(info.name, it->second);
				break;
			}
			}
		};
		if (shader != nullptr) {
			m_material->m_settingsBufferData.Resize(shader->PropertyBufferAlignedSize());
			for (size_t i = 0u; i < shader->PropertyCount(); i++)
				restorePropertyValue(shader->Property(i));
		}
		else m_material->m_settingsBufferData.Resize(1u);

		m_flags |= FLAG_FIELDS_DIRTY;
		m_flags |= FLAG_SHADER_DIRTY;
	}

	void Material::Writer::GetFields(const Callback<Serialization::SerializedObject>& recordElement) {
		// Serialize shader:
		{
			const Reference<const LitShader> initialShader = m_material->m_shader;
			const LitShader* shaderPtr = initialShader;
			Reference<const LitShaderSet> shaderSet = LitShaderSet::All();
			assert(shaderSet != nullptr);
			recordElement(shaderSet->LitShaderSelector()->Serialize(shaderPtr));
			if (shaderPtr != initialShader)
				SetShader(shaderPtr);
		}

		// Serialize fields:
		if (m_material->m_shader != nullptr)
			for (size_t fieldId = 0u; fieldId < m_material->m_shader->PropertyCount(); fieldId++) {
				const PropertyInfo& info = m_material->m_shader->Property(fieldId);
				static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<decltype(info)>>, PropertyInfo>);
				auto serializeAs = [&](auto& defaultVal) -> bool {
					using Type = std::remove_const_t<std::remove_reference_t<decltype(defaultVal)>>;
					using StoredType = std::conditional_t<std::is_same_v<bool, Type>, uint32_t, Type>;
					::Jimara::Unused(defaultVal);
					StoredType& settingsValue = *reinterpret_cast<StoredType*>(m_material->m_settingsBufferData.Data() + info.settingsBufferOffset);
					Type value = static_cast<Type>(settingsValue);
					const Serialization::ItemSerializer::Of<Type>* serializer =
						dynamic_cast<const Serialization::ItemSerializer::Of<Type>*>(info.serializer.operator->());
					assert(serializer != nullptr);
					recordElement(serializer->Serialize(value));
					if (static_cast<StoredType>(value) == settingsValue)
						return false;
					settingsValue = static_cast<StoredType>(value);
					m_flags |= FLAG_FIELDS_DIRTY;
					return true;
				};
				auto serializeField = [&]() {
					switch (info.type) {
					case PropertyType::FLOAT:
						return serializeAs(info.defaultValue.fp32);
					case PropertyType::DOUBLE:
						return serializeAs(info.defaultValue.fp64);
					case PropertyType::INT32:
						return serializeAs(info.defaultValue.int32);
					case PropertyType::UINT32:
						return serializeAs(info.defaultValue.uint32);
					case PropertyType::INT64:
						return serializeAs(info.defaultValue.int64);
					case PropertyType::UINT64:
						return serializeAs(info.defaultValue.uint64);
					case PropertyType::BOOL32:
						return serializeAs(info.defaultValue.bool32);
					case PropertyType::VEC2:
						return serializeAs(info.defaultValue.vec2);
					case PropertyType::VEC3:
						return serializeAs(info.defaultValue.vec3);
					case PropertyType::VEC4:
						return serializeAs(info.defaultValue.vec4);
					case PropertyType::MAT4:
						return serializeAs(info.defaultValue.mat4);
					case PropertyType::SAMPLER2D:
					{
						const Reference<Graphics::TextureSampler> initialValue = GetPropertyValue<Graphics::TextureSampler*>(info.name);
						Reference<Graphics::TextureSampler> sampler = initialValue;
						const Serialization::DefaultSerializer<Reference<Graphics::TextureSampler>>::Serializer_T* serializer =
							dynamic_cast<const Serialization::DefaultSerializer<Reference<Graphics::TextureSampler>>::Serializer_T*>(info.serializer.operator->());
						assert(serializer != nullptr);
						recordElement(serializer->Serialize(sampler));
						if (initialValue != sampler)
							SetPropertyValue(info.name, sampler);
						return initialValue != sampler;
					}
					default:
						return false;
					}
				};
				serializeField();
			}
	}
}
}
