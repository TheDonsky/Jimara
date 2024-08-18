#pragma once
namespace Jimara {
namespace Refactor {
	class Material;
}
}
#include "../../Graphics/GraphicsDevice.h"


namespace Jimara {
namespace Refactor {
	class JIMARA_API Material : public virtual Resource {
	public:
		enum class PropertyType : uint8_t;
		union PropertyValue;
		struct PropertyInfo;
		struct Property;

		class LitShader;
		class Reader;
		class Writer;
		class Instance;
		class CachedInstance;

		/// <summary> Name of the main settings buffer binding </summary>
		static const constexpr std::string_view SETTINGS_BUFFER_BINDING_NAME = "JM_MaterialSettingsBuffer";

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

		/// <summary> Size of a field of given type within property buffer </summary>
		static size_t PropertySize(PropertyType type);

		/// <summary> Alignment of a field of given type within property buffer </summary>
		static size_t PropertyAlignment(PropertyType type);

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// Bindless structured buffers
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> m_bindlessBuffers;

		// Bindless textures
		const Reference<Graphics::BindlessSet<Graphics::TextureSampler>> m_bindlessSamplers;

		// Shared lock for readers/writers
		mutable std::shared_mutex m_readWriteLock;

		// Shader
		Reference<const LitShader> m_shader;

		// Settings buffer
		Stacktor<uint8_t> m_settingsBufferData;
		const Reference<Graphics::ResourceBinding<Graphics::Buffer>> m_settingsConstantBuffer = 
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>();
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_settingsBufferId;

		// Image bindings
		struct JIMARA_API ImageBinding : public virtual Object {
			std::string bindingName;
			Reference<Graphics::ResourceBinding<Graphics::TextureSampler>> samplerBinding;
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> samplerId;
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
		uint32_t bool32;
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

		inline static const constexpr size_t NO_ID = ~size_t(0u);

		size_t PropertyIdByName(const std::string_view& name)const;

		size_t PropertyIdByBindingName(const std::string_view& bindingName)const;

	private:
		OS::Path m_shaderPath;
		std::vector<PropertyInfo> m_properties;
		size_t m_propertyBufferSize = 0;
		size_t m_propertyBufferAlignment = 1;
		std::unordered_map<std::string_view, size_t> m_propertyIdByName;
		std::unordered_map<std::string_view, size_t> m_propertyIdByBindingName;

		inline LitShader(const LitShader&) = delete;
		inline LitShader& operator=(const LitShader&) = delete;
		inline LitShader(LitShader&&) = delete;
		inline LitShader& operator=(LitShader&&) = delete;
	};

	/*
	class JIMARA_API Material::Writer final {
	public:
		Writer(Material* material);

		virtual ~Writer();

		template<typename Type>
		inline Type GetSettingsBufferContent() { return *reinterpret_cast<const Type*>(GetSettingsBufferData(sizeof(Type))); }

		template<typename Type>
		void SetSettingsBufferContent(const Type& value) { GetSettingsBufferData(reinterpret_cast<const void*>(&value), sizeof(Type)); }

		Graphics::TextureSampler* GetSampler(const std::string_view& bindingName);

		void SetSampler(const std::string_view& bindingName, Graphics::TextureSampler* sampler);

	private:
		std::unique_lock<std::shared_mutex> m_lock;
		uint8_t m_flags = 0;

		const void* GetSettingsBufferData(size_t numBytes);
		void SetSettingsBufferData(const void* data, size_t numBytes);

		struct Helpers;
	};
	*/
}
}
