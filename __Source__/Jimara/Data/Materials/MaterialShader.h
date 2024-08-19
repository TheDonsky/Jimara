#pragma once
namespace Jimara {
namespace Refactor {
	class Material;
}
}
#include "../../Graphics/GraphicsDevice.h"
#include "../../Graphics/Pipeline/OneTimeCommandPool.h"
#include "../../Graphics/Data/ShaderBinaries/ShaderClass.h"
#include "../Serialization/Serializable.h"


namespace Jimara {
namespace Refactor {
	class JIMARA_API Material : public virtual Resource, public virtual Serialization::Serializable {
	public:
		enum class PropertyType : uint8_t;
		union PropertyValue;
		struct PropertyInfo;
		struct Property;

		class LitShader;
		class LitShaderSet;

		/// <summary> Shader class selector </summary>
		typedef Serialization::ItemSerializer::Of<const LitShader*> LitShaderSerializer;

		class Reader;
		class Writer;
		
		class Instance;
		class CachedInstance;

		/// <summary> Name of the main settings buffer binding </summary>
		static const constexpr std::string_view SETTINGS_BUFFER_BINDING_NAME = "JM_MaterialSettingsBuffer";

		/// <summary> 'Not found'/'missing' value for all index lookups </summary>
		inline static const constexpr size_t NO_ID = ~size_t(0u);

		/// <summary> Graphics device </summary>
		inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

		/// <summary> Bindless structured buffers </summary>
		inline Graphics::BindlessSet<Graphics::ArrayBuffer>* BindlessBuffers()const { return m_bindlessBuffers; }

		/// <summary> Bindless texture samplers </summary>
		inline Graphics::BindlessSet<Graphics::TextureSampler>* BindlessSamplers()const { return m_bindlessSamplers; }

		/// <summary> Invoked, whenever any of the material properties gets altered </summary>
		inline Event<const Material*>& OnMaterialDirty()const { return m_onMaterialDirty; }

		/// <summary> Invoked, whenever the shared instance gets invalidated </summary>
		inline Event<const Material*>& OnInvalidateSharedInstance()const { return m_onInvalidateSharedInstance; }

		/// <summary>
		/// Gives access to material fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary> Size of a field of given type within property buffer </summary>
		static size_t PropertySize(PropertyType type);

		/// <summary> Alignment of a field of given type within property buffer </summary>
		static size_t PropertyAlignment(PropertyType type);

		/// <summary>
		/// Translates PropertyType to TypeId
		/// <para/> For referenced types, just returns the pointer-type, not reference (ei Graphics::TextureSampler*, for example)
		/// </summary>
		static TypeId PropertyTypeId(PropertyType type);

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// Bindless structured buffers
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> m_bindlessBuffers;

		// Bindless textures
		const Reference<Graphics::BindlessSet<Graphics::TextureSampler>> m_bindlessSamplers;

		// One-time command buffers
		const Reference<Graphics::OneTimeCommandPool> m_oneTimeCommandBuffers;

		// Shared lock for readers/writers
		mutable std::shared_mutex m_readWriteLock;

		// Shader
		Reference<const LitShader> m_shader;

		// Settings buffer
		Stacktor<uint8_t> m_settingsBufferData;
		Reference<Graphics::Buffer> m_settingsConstantBuffer;
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_settingsBufferId;

		// Image bindings
		struct JIMARA_API ImageBinding : public virtual Object {
			std::string bindingName;
			bool isDefault = false;
			Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> samplerBinding;
			Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> samplerId;
		};
		std::unordered_map<std::string_view, Reference<ImageBinding>> m_imageByBindingName;

		// Lock for preventing multi-initialisation of m_sharedInstance
		mutable std::mutex m_instanceLock;

		// Invoked, whenever any of the material properties gets altered
		mutable EventInstance<const Material*> m_onMaterialDirty;

		// Invoked, whenever the shared instance gets invalidated
		mutable EventInstance<const Material*> m_onInvalidateSharedInstance;

		// Private stuff resides in-here...
		struct Helpers;
	};


	enum class JIMARA_API Material::PropertyType : uint8_t {
		FLOAT,
		DOUBLE,
		INT32,
		UINT32,
		INT64,
		UINT64,
		BOOL32,
		VEC2,
		VEC3,
		VEC4,
		MAT4,
		SAMPLER2D
	};
	static_assert(std::is_same_v<int, int32_t>);

	union JIMARA_API Material::PropertyValue {
		float fp32;
		double fp64;
		int32_t int32;
		uint32_t uint32;
		int64_t int64;
		uint64_t uint64;
		bool bool32;
		Vector2 vec2;
		Vector3 vec3;
		Vector4 vec4;
		Matrix4 mat4;
		Vector4 samplerColor;
	};

	struct JIMARA_API Material::Property {
		std::string name;
		std::string alias;
		std::string hint;
		PropertyType type = {};
		PropertyValue defaultValue = {};

		static Property Float(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, float defaultValue);
		static Property Double(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, double defaultValue);
		static Property Int32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int32_t defaultValue);
		static Property Uint32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint32_t defaultValue);
		static Property Int64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int64_t defaultValue);
		static Property Uint64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint64_t defaultValue);
		static Property Bool32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, bool defaultValue);
		static Property Vec2(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector2 defaultValue);
		static Property Vec3(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector3 defaultValue);
		static Property Vec4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue);
		static Property Mat4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Matrix4 defaultValue);
		static Property Sampler2D(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue);
	};

	struct JIMARA_API Material::PropertyInfo final : Material::Property {
		std::string bindingName;
		size_t settingsBufferOffset = 0u;
		Reference<const Serialization::ItemSerializer> serializer;
	};

	class JIMARA_API Material::LitShader : public virtual Object {
	public:
		LitShader(const OS::Path& litShaderPath, const std::vector<Material::Property>& properties);

		inline virtual ~LitShader() {}

		inline const OS::Path& LitShaderPath()const { return m_shaderPath; }

		inline size_t PropertyCount()const { return m_properties.size(); }

		inline const PropertyInfo& Property(size_t index)const { return m_properties[index]; }

		inline size_t PropertyBufferSize()const { return m_propertyBufferSize; }

		inline size_t PropertyBufferAlignment()const { return m_propertyBufferAlignment; }

		inline size_t PropertyBufferAlignedSize()const { return ((m_propertyBufferSize + m_propertyBufferAlignment - 1u) / m_propertyBufferAlignment) * m_propertyBufferAlignment; }

		size_t PropertyIdByName(const std::string_view& name)const;

		size_t PropertyIdByBindingName(const std::string_view& bindingName)const;

	private:
		OS::Path m_shaderPath;
		std::string m_pathStr;
		std::vector<PropertyInfo> m_properties;
		size_t m_propertyBufferSize = 0;
		size_t m_propertyBufferAlignment = 1;
		std::unordered_map<std::string_view, size_t> m_propertyIdByName;
		std::unordered_map<std::string_view, size_t> m_propertyIdByBindingName;

		inline LitShader(const LitShader&) = delete;
		inline LitShader& operator=(const LitShader&) = delete;
		inline LitShader(LitShader&&) = delete;
		inline LitShader& operator=(LitShader&&) = delete;
		friend class LitShaderSet;
	};

	class JIMARA_API Material::LitShaderSet : public virtual Object {
	public:
		/// <summary>
		///  Set of all currently registered LitShader objects
		/// Notes: 
		///		0. Value will automagically change whenever any new type gets registered or unregistered;
		///		1. LitShaderSet is immutable, so no worries about anything going out of scope or deadlocking.
		/// </summary>
		/// <returns> Reference to current set of all </returns>
		static Reference<LitShaderSet> All();

		/// <summary> Number of shaders within the set </summary>
		size_t Size()const;

		/// <summary>
		/// LitShader by index
		/// </summary>
		/// <param name="index"> LitShader index </param>
		/// <returns> LitShader </returns>
		const LitShader* At(size_t index)const;

		/// <summary>
		/// LitShader by index
		/// </summary>
		/// <param name="index"> LitShader index </param>
		/// <returns> LitShader </returns>
		const LitShader* operator[](size_t index)const;

		/// <summary>
		/// Finds LitShader by it's ShaderPath() value
		/// </summary>
		/// <param name="shaderPath"> Shader path within the code </param>
		/// <returns> LitShader s, such that s->ShaderPath() is shaderPath if found, nullptr otherwise </returns>
		const LitShader* FindByPath(const OS::Path& shaderPath)const;

		/// <summary>
		/// Finds index of a shader class
		/// </summary>
		/// <param name="litShader"> Shader class </param>
		/// <returns> Index i, such that this->At(i) is litShader; NoIndex if not found </returns>
		size_t IndexOf(const LitShader* litShader)const;

		/// <summary> Shader class selector (Alive and valid only while this set exists..) </summary>
		const LitShaderSerializer* LitShaderSelector()const { return m_classSelector; }

	private:
		// List of all known shaders
		const std::vector<Reference<const LitShader>> m_shaders;

		// Index of each shader
		typedef std::unordered_map<const LitShader*, size_t> IndexPerShader;
		const IndexPerShader m_indexPerShader;

		// Mapping from LitShader::ShaderPath() to corresponding LitShader()
		typedef std::unordered_map<OS::Path, const LitShader*> ShadersByPath;
		const ShadersByPath m_shadersByPath;

		// Shader class selector
		const Reference<const LitShaderSerializer> m_classSelector;

		// Constructor
		LitShaderSet(const std::set<Reference<const LitShader>>& shaders);
	};

	
	class JIMARA_API Material::Writer final {
	public:
		Writer(Material* material);

		virtual ~Writer();

		inline const LitShader* Shader()const { return (m_material == nullptr) ? nullptr : m_material->m_shader.operator->(); }

		void SetShader(const LitShader* shader);

		template<typename ValueType>
		inline ValueType GetPropertyValue(const std::string_view& materialPropertyName)const;

		template<typename ValueType>
		inline void SetPropertyValue(const std::string_view& materialFieldName, const ValueType& value);

		void GetFields(const Callback<Serialization::SerializedObject>& recordElement);

	private:
		const Reference<Material> m_material;
		static const constexpr uint8_t FLAG_FIELDS_DIRTY = 1;
		static const constexpr uint8_t FLAG_SHADER_DIRTY = 2;
		uint8_t m_flags = 0;

		struct Helpers;
	};

	template<typename ValueType>
	inline ValueType Material::Writer::GetPropertyValue(const std::string_view& materialPropertyName)const {
		if (m_material == nullptr || m_material->m_shader == nullptr) 
			return ValueType(0);
		size_t propertyId = m_material->m_shader->PropertyIdByName(materialPropertyName);
		if (propertyId == NO_ID) 
			return ValueType(0);
		const PropertyInfo& property = m_material->m_shader->Property(propertyId);
		if (property.type == PropertyType::SAMPLER2D) {
			const constexpr bool isSamplerReferenceType = std::is_same_v<ValueType, Graphics::TextureSampler*> || std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>;
			if (isSamplerReferenceType) {
				static_assert(sizeof(size_t) == sizeof(Graphics::TextureSampler*));
				const auto it = m_material->m_imageByBindingName.find(property.bindingName);
				assert(it != m_material->m_imageByBindingName.end());
				if (it->second->isDefault)
					return ValueType(0);
				Graphics::TextureSampler* const samplerAddress = static_cast<Graphics::TextureSampler*>(it->second->samplerBinding->BoundObject());
				static_assert(std::is_same_v<std::conditional_t<sizeof(uint32_t) < sizeof(uint64_t), uint32_t, size_t>, uint32_t>);
				const auto ptrVal = static_cast<std::conditional_t<isSamplerReferenceType, size_t, float>>((size_t)samplerAddress);
				return ValueType(ptrVal);
			}
			else {
				m_material->GraphicsDevice()->Log()->Error(
					"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a texture sampler! [", __FILE__, "; ", __LINE__, "]");
				return ValueType(0);
			}
		}
		else if (PropertyTypeId(property.type) != TypeId::Of<ValueType>()) {
			m_material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a ",
				PropertyTypeId(property.type).Name(), ", not a ", TypeId::Of<ValueType>().Name(), "! [", __FILE__, "; ", __LINE__, "]");
			return ValueType(0);
		}
		else return *reinterpret_cast<const ValueType*>(m_material->m_settingsBufferData.Data() + property.settingsBufferOffset);
	}

	template<typename ValueType>
	inline void Material::Writer::SetPropertyValue(const std::string_view& materialPropertyName, const ValueType& value) {
		if (m_material == nullptr || m_material->m_shader == nullptr)
			return;
		size_t propertyId = m_material->m_shader->PropertyIdByName(materialPropertyName);
		if (propertyId == NO_ID)
			return;
		const PropertyInfo& property = m_material->m_shader->Property(propertyId);
		if (property.type == PropertyType::SAMPLER2D) {
			if (std::is_same_v<ValueType, Graphics::TextureSampler*> || std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>) {
				static_assert(sizeof(size_t) == sizeof(Graphics::TextureSampler*));
				const auto it = m_material->m_imageByBindingName.find(property.bindingName);
				assert(it != m_material->m_imageByBindingName.end());
				Graphics::TextureSampler* samplerValue;
				const ValueType* val = &value;
				if (std::is_same_v<ValueType, Graphics::TextureSampler*>)
					samplerValue = (*reinterpret_cast<Graphics::TextureSampler* const *>(val));
				else if (std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>)
					samplerValue = (*reinterpret_cast<const Reference<Graphics::TextureSampler>*>(val));
				else samplerValue = nullptr;
				if ((it->second->isDefault) && it->second->samplerBinding != nullptr && it->second->samplerBinding->BoundObject() == samplerValue)
					return;
				else if (samplerValue == nullptr) {
					it->second->isDefault = true;
					it->second->samplerBinding = Graphics::ShaderClass::SharedTextureSamplerBinding(property.defaultValue.vec4, m_material->GraphicsDevice());
				}
				else {
					it->second->isDefault = false;
					it->second->samplerBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(samplerValue);
				}
				if (it->second->samplerId->BoundObject() == samplerValue)
					return;
				it->second->samplerId = m_material->m_bindlessSamplers->GetBinding(samplerValue);
				assert(it->second->samplerId != nullptr);
				(*reinterpret_cast<uint32_t*>(m_material->m_settingsBufferData.Data() + property.settingsBufferOffset)) = it->second->samplerId->Index();
				m_flags |= FLAG_FIELDS_DIRTY;
			}
			else m_material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a texture sampler! [", __FILE__, "; ", __LINE__, "]");
		}
		else if (PropertyTypeId(property.type) != TypeId::Of<ValueType>())
			m_material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a ",
				PropertyTypeId(property.type).Name(), ", not a ", TypeId::Of<ValueType>().Name(), "! [", __FILE__, "; ", __LINE__, "]");
		else {
			ValueType& curVal = (*reinterpret_cast<ValueType*>(m_material->m_settingsBufferData.Data() + property.settingsBufferOffset));
			if (curVal == value)
				return;
			curVal = value;
			m_flags |= FLAG_FIELDS_DIRTY;
		}
	}
}
}
