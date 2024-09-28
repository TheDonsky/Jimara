#pragma once
namespace Jimara {
	class Material;
}
#include "../../Graphics/GraphicsDevice.h"
#include "../../Graphics/Pipeline/OneTimeCommandPool.h"
#include "../../Graphics/Data/ConstantResources.h"
#include "../Serialization/Serializable.h"


namespace Jimara {
	/// <summary>
	/// Material, describing shader and resources, that can be applied to a rendered object
	/// Note: Material property reads and writes are performed using Material::Reader and Material::Writer to ensure thread-safety; 
	/// read-up on them for further details
	/// </summary>
	class JIMARA_API Material : public virtual Resource {
	public:
		enum class PropertyType : uint8_t;
		union PropertyValue;
		struct PropertyInfo;
		struct Property;

		struct EditorPath;
		enum class BlendMode : uint32_t;
		enum class MaterialFlags : uint32_t;

		class LitShader;
		class LitShaderSet;

		class Reader;
		class Writer;
		class Serializer;
		
		class Instance;
		class CachedInstance;

		/// <summary> Shader class selector </summary>
		typedef Serialization::ItemSerializer::Of<const LitShader*> LitShaderSerializer;

		/// <summary> Name of the main settings buffer binding </summary>
		static const constexpr std::string_view SETTINGS_BUFFER_BINDING_NAME = "jm_MaterialSettingsBuffer";

		/// <summary> 'Not found'/'missing' value for all index lookups </summary>
		inline static const constexpr size_t NO_ID = ~size_t(0u);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="graphicsDevice"> Graphics device </param>
		/// <param name="bindlessBuffers"> Bindless structured buffer set </param>
		/// <param name="bindlessSamplers"> Bindless texture sampler set </param>
		Material(Graphics::GraphicsDevice* graphicsDevice,
			Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
			Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers);

		/// <summary> Virtual destructor </summary>
		virtual ~Material();

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
		/// <param name="shaderSelector"> Lit-shader selector </param>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		void GetFields(const LitShaderSerializer* shaderSelector, Callback<Serialization::SerializedObject> recordElement);

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
		mutable SpinLock m_instanceLock;

		// Shared instance
		mutable Reference<Instance> m_sharedInstance;

		// Invoked, whenever any of the material properties gets altered
		mutable EventInstance<const Material*> m_onMaterialDirty;

		// Invoked, whenever the shared instance gets invalidated
		mutable EventInstance<const Material*> m_onInvalidateSharedInstance;

		// Private stuff resides in-here...
		struct Helpers;

		// Property getter body
		template<typename ValueType>
		inline static ValueType GetPropertyValue(const Material* material, const std::string_view& materialPropertyName);

		// Property setter body
		template<typename ValueType>
		inline static bool SetPropertyValue(Material* material, const std::string_view& materialFieldName, const ValueType& value);
	};


	/// <summary> 
	/// Material properties, as defined in .jlm files will be broken down into Material::Property fields, each of which is of one of these types
	/// <para/> Structures, when supported, will also be broken down on a field-level to these few choices.
	/// </summary>
	enum class JIMARA_API Material::PropertyType : uint8_t {
		/// <summary> float </summary>
		FLOAT = 0,

		/// <summary> double </summary>
		DOUBLE = 1,

		/// <summary> int/int32_t </summary>
		INT32 = 2,
		
		/// <summary> uint/unsigned int/uint32_t </summary>
		UINT32 = 3,

		/// <summary> int64_t </summary>
		INT64 = 4,

		/// <summary> uint64_t </summary>
		UINT64 = 5,

		/// <summary> bool32(glsl) or bool (c++) </summary>
		BOOL32 = 6,

		/// <summary> vec2(glsl) or Vector2 (C++) </summary>
		VEC2 = 7,

		/// <summary> vec3(glsl) or Vector3 (C++) </summary>
		VEC3 = 8,

		/// <summary> vec4(glsl) or Vector4 (C++) </summary>
		VEC4 = 9,

		/// <summary> mat4(glsl) or Matrix4 (C++) </summary>
		MAT4 = 10,

		/// <summary> sampler2D(glsl) or Graphics::TextureSampler* (C++) </summary>
		SAMPLER2D = 11,

		/// <summary> Generated 32 bit padding (ignored during serialization and you can not read/write from CPU easily; just used for internal calculations) </summary>
		PAD32 = 12
	};
	static_assert(std::is_same_v<int, int32_t>);


	/// <summary>
	/// Union of property values or default values
	/// </summary>
	union JIMARA_API Material::PropertyValue {
		/// <summary> float </summary>
		float fp32;

		/// <summary> double </summary>
		double fp64;

		/// <summary> int/int32_t </summary>
		int32_t int32;

		/// <summary> uint/unsigned int/uint32_t </summary>
		uint32_t uint32;

		/// <summary> int64_t </summary>
		int64_t int64;

		/// <summary> uint64_t </summary>
		uint64_t uint64;

		/// <summary> bool32(glsl) or bool (c++) </summary>
		bool bool32;

		/// <summary> vec2(glsl) or Vector2 (C++) </summary>
		Vector2 vec2;

		/// <summary> vec3(glsl) or Vector3 (C++) </summary>
		Vector3 vec3;

		/// <summary> vec4(glsl) or Vector4 (C++) </summary>
		Vector4 vec4;

		/// <summary> mat4(glsl) or Matrix4 (C++) </summary>
		Matrix4 mat4;

		/// <summary> Fixed/default-color value of sampler2D(glsl) or Graphics::TextureSampler* (C++) (irrelevant when texture is set) </summary>
		Vector4 samplerColor;
	};


	/// <summary>
	/// Material propery definition
	/// <para/> These will be auto-generated by the precompiler alongside the corresponding LitShader instances
	/// </summary>
	struct JIMARA_API Material::Property {
		/// <summary> 
		/// Property variable name as defined in .jls file;
		/// <para/> For structures, when supported, the fields will be split into multiple properties on per-field basis and these names will look like "variableName.fieldName"
		/// </summary>
		std::string name;

		/// <summary> Property name alias to display in-editor </summary>
		std::string alias;

		/// <summary> Hint about the property or it's description to disaplay in-editor </summary>
		std::string hint;

		/// <summary> Property field type </summary>
		PropertyType type = PropertyType::PAD32;

		/// <summary> Default value union </summary>
		PropertyValue defaultValue = {};

		/// <summary> Property attributes </summary>
		std::vector<Reference<const Object>> attributes;

		/// <summary>
		/// Creates a float-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Float(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, float defaultValue);

		/// <summary>
		/// Creates a double-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Double(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, double defaultValue);

		/// <summary>
		/// Creates a int32_t-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Int32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int32_t defaultValue);

		/// <summary>
		/// Creates a uint32_t-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Uint32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint32_t defaultValue);

		/// <summary>
		/// Creates a int64_t-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Int64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, int64_t defaultValue);

		/// <summary>
		/// Creates a uint64_t-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Uint64(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, uint64_t defaultValue);

		/// <summary>
		/// Creates a bool-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Bool32(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, bool defaultValue);

		/// <summary>
		/// Creates a Vector2-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Vec2(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector2 defaultValue);

		/// <summary>
		/// Creates a Vector3-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Vec3(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector3 defaultValue);

		/// <summary>
		/// Creates a Vector4-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Vec4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue);

		/// <summary>
		/// Creates a Matrix4-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Mat4(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Matrix4 defaultValue);

		/// <summary>
		/// Creates a sampler-type property
		/// </summary>
		/// <param name="name"> Property variable name </param>
		/// <param name="alias"> Property name alias to display in-editor </param>
		/// <param name="hint"> Hint about the property or it's description to disaplay in-editor </param>
		/// <param name="defaultValue"> Property field type </param>
		/// <returns> Property definition </returns>
		static Property Sampler2D(const std::string_view& name, const std::string_view& alias, const std::string_view& hint, Vector4 defaultValue);

		/// <summary> Serializer for Property </summary>
		struct Serializer;

		/// <summary>
		/// Compares to other property
		/// </summary>
		/// <param name="other"> Property to compare to </param>
		/// <returns> (*this) == other </returns>
		inline bool operator==(const Property& other)const { 
			return (name == other.name) &&
				(alias == other.alias) &&
				(hint == other.hint) &&
				(type == other.type) &&
				(std::memcmp((const void*)&defaultValue, (const void*)&other.defaultValue, sizeof(PropertyValue))) &&
				(attributes == other.attributes);
		}

		/// <summary>
		/// Compares to other property
		/// </summary>
		/// <param name="other"> Property to compare to </param>
		/// <returns> (*this) != other </returns>
		inline bool operator!=(const Property& other)const { return !((*this) == other); }
	};


	/// <summary>
	/// Material Property field, alongside it's direct-binding name, settings buffer offset and a serializer 
	/// </summary>
	struct JIMARA_API Material::PropertyInfo final : Material::Property {
		/// <summary> Name of a binding, applicable for lighting models where JM_DefineDirectMaterialBindings is used </summary>
		std::string bindingName;

		/// <summary> Value offset within JM_MaterialProperties_Buffer glsl struct </summary>
		size_t settingsBufferOffset = 0u;

		/// <summary> Value serializer (Of corresponding C++ type) </summary>
		Reference<const Serialization::ItemSerializer> serializer;
	};


	/// <summary>
	/// Each lit shader defines it's public name and path for the editor as follows
	/// </summary>
	struct Material::EditorPath {
		/// <summary> Shader name/alias for the editor </summary>
		std::string name;

		/// <summary> Shader path for the editor selector </summary>
		std::string path;

		/// <summary> Shader hint for the editor </summary>
		std::string hint;

		/// <summary> Serializer for EditorPath </summary>
		struct Serializer;

		/// <summary>
		/// Compares to other path
		/// </summary>
		/// <param name="other"> Path to compare to </param>
		/// <returns> (*this) == other </returns>
		inline bool operator==(const EditorPath& other)const { return (name == other.name) && (path == other.path) && (hint == other.hint); }

		/// <summary>
		/// Compares to other path
		/// </summary>
		/// <param name="other"> Path to compare to </param>
		/// <returns> (*this) != other </returns>
		inline bool operator!=(const EditorPath& other)const { return !((*this) == other); }
	};

	/// <summary>
	/// Each lit shader defines it's blending mode as follows
	/// </summary>
	enum class Material::BlendMode : uint32_t {
		/// <summary> JM_Blend_Opaque </summary>
		Opaque =	0,

		/// <summary> JM_Blend_Alpha </summary>
		Alpha =		1,
		
		/// <summary> JM_Blend_Additive </summary>
		Additive =	2
	};

	/// <summary>
	/// Lit shaders define which optional vertex inputs to use, as well as some other optimization and/or features to use through JM_MaterialFlags
	/// </summary>
	enum class Material::MaterialFlags : uint32_t {
		/// <summary> No optional features/hints </summary>
		None = 0,

		/// <summary> JM_CanDiscard (used for stuff like cutout-type materials and alike; Allows fragment discard when JM_Init fails) </summary>
		CanDiscard =					(1 << 0),

		/// <summary> JM_UseObjectId (Exposes JM_ObjectIndex through JM_VertexInput) </summary>
		UseObjectId =					(1 << 1),

		/// <summary> JM_UsePerVertexTilingAndOffset (Exposes JM_ObjectTilingAndOffset through JM_VertexInput) </summary>
		UsePerVertexTilingAndOffset =	(1 << 2),

		/// <summary> JM_UseVertexColor (Exposes JM_VertexColor through JM_VertexInput) </summary>
		UseVertexColor =				(1 << 3),

		/// <summary> JM_UseTangents (Exposes derived tangent and bitangent vectors through JM_VertexInput) </summary>
		UseTangents =					(1 << 4)
	};


	/// <summary>
	/// Lit-shader definition
	/// <para/> Lit-shader definitions will be generated by preprocessor and shader compiler and constructed when creating ShaderLibrary objets; 
	/// User does not need to code these by hand!
	/// </summary>
	class JIMARA_API Material::LitShader : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// <para/> Packed JM_MaterialProperties_Buffer structure is defined strictly based on the properties list;
		/// Because of that, properties order HAS TO MATCH that of the .jls file;
		/// Fortunately, these are generated by preprocessor and we don't need to care about them unless we do :D
		/// </summary>
		/// <param name="litShaderPath"> Path to the shader </param>
		/// <param name="editorPaths"> Paths for the editor selector </param>
		/// <param name="blendMode"> Shader blend mode </param>
		/// <param name="materialFlags"> Optional vertex input requirenments, as well as some other optimization and/or features </param>
		/// <param name="shadingStateSize"> JM_ShadingStateSize within the shader </param>
		/// <param name="properties"> Property fields </param>
		LitShader(
			const OS::Path& litShaderPath, const std::vector<Material::EditorPath>& editorPaths,
			Material::BlendMode blendMode, Material::MaterialFlags materialFlags, size_t shadingStateSize,
			const std::vector<Material::Property>& properties);

		/// <summary> Virtual destructor </summary>
		inline virtual ~LitShader() {}

		/// <summary> Path to the shader (for loading) </summary>
		inline const OS::Path& LitShaderPath()const { return m_shaderPath; }

		/// <summary> Number of editor paths </summary>
		inline size_t EditorPathCount()const { return m_editorPaths.size(); }

		/// <summary>
		/// Editor path by id
		/// </summary>
		/// <param name="index"> Index of the editor-path </param>
		/// <returns> Editor path </returns>
		inline const Material::EditorPath& EditorPath(size_t index)const { return m_editorPaths[index]; }

		/// <summary> Shader blend mode </summary>
		inline Material::BlendMode BlendMode()const { return m_blendMode; }

		/// <summary> Optional vertex input requirenments, as well as some other optimization and/or features </summary>
		inline Material::MaterialFlags MaterialFlags()const { return m_materialFlags; }

		/// <summary> JM_ShadingStateSize within the shader </summary>
		inline size_t ShadingStateSize()const { return m_shadingStateSize; }

		/// <summary> Number of property fields (these may include paddings, so be wary when using) </summary>
		inline size_t PropertyCount()const { return m_properties.size(); }

		/// <summary>
		/// Property field by index (these may include paddings, so be wary when using)
		/// </summary>
		/// <param name="index"> Property index </param>
		/// <returns> Property details </returns>
		inline const PropertyInfo& Property(size_t index)const { return m_properties[index]; }

		/// <summary> Size of JM_MaterialProperties_Buffer structure </summary>
		inline size_t PropertyBufferSize()const { return m_propertyBufferSize; }

		/// <summary> Required padding of JM_MaterialProperties_Buffer structure </summary>
		inline size_t PropertyBufferAlignment()const { return m_propertyBufferAlignment; }

		/// <summary> "Padded Size" of JM_MaterialProperties_Buffer structure </summary>
		inline size_t PropertyBufferAlignedSize()const { 
			return ((m_propertyBufferSize + m_propertyBufferAlignment - 1u) / m_propertyBufferAlignment) * m_propertyBufferAlignment; 
		}

		/// <summary>
		/// Searches for a property by name
		/// </summary>
		/// <param name="name"> Property name </param>
		/// <returns> Property index if found, NO_ID otherwise </returns>
		size_t PropertyIdByName(const std::string_view& name)const;

		/// <summary>
		/// Searches for a sampler property index by name (does not work for non-samplers!) 
		/// </summary>
		/// <param name="bindingName"> Binding name for lighting models where JM_DefineDirectMaterialBindings is used </param>
		/// <returns> Sampler Property index if found, NO_ID otherwise </returns>
		size_t PropertyIdByBindingName(const std::string_view& bindingName)const;


	private:
		// Path to the shader
		OS::Path m_shaderPath;
		std::string m_pathStr;
		std::vector<Material::EditorPath> m_editorPaths;

		// Options and flags
		Material::BlendMode m_blendMode;
		Material::MaterialFlags m_materialFlags;
		size_t m_shadingStateSize;

		// List of properties
		std::vector<PropertyInfo> m_properties;

		// JM_MaterialProperties_Buffer size
		size_t m_propertyBufferSize = 0;

		// JM_MaterialProperties_Buffer alignment requirenment
		size_t m_propertyBufferAlignment = 1;

		// Property lookup table
		std::unordered_map<std::string_view, size_t> m_propertyIdByName;
		std::unordered_map<std::string_view, size_t> m_propertyIdByBindingName;

		// Copy-move are disabled
		inline LitShader(const LitShader&) = delete;
		inline LitShader& operator=(const LitShader&) = delete;
		inline LitShader(LitShader&&) = delete;
		inline LitShader& operator=(LitShader&&) = delete;
		friend class LitShaderSet;
	};


	/// <summary>
	/// Collection of [available] Material::LitShaders
	/// </summary>
	class JIMARA_API Material::LitShaderSet : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shaders"> Set of available lit-shaders </param>
		LitShaderSet(const std::set<Reference<const LitShader>>& shaders);

		/// <summary> Virtual destructor </summary>
		inline virtual ~LitShaderSet() {}

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
		inline const LitShaderSerializer* LitShaderSelector()const { return m_classSelector; }

		/// <summary> Shared instance of a material serializer </summary>
		inline const Material::Serializer* MaterialSerializer()const { return m_materialSerializer; }

	private:
		// List of all known shaders
		const std::vector<Reference<const LitShader>> m_shaders;

		// Index of each shader
		typedef std::unordered_map<const LitShader*, size_t> IndexPerShader;
		const IndexPerShader m_indexPerShader;

		// Mapping from LitShader::ShaderPath() to corresponding LitShader()
		typedef std::unordered_map<OS::Path, Reference<const LitShader>> ShadersByPath;
		const std::shared_ptr<const ShadersByPath> m_shadersByPath;

		// Shader class selector
		Reference<const LitShaderSerializer> m_classSelector;

		// Shader serializer
		Reference<const Material::Serializer> m_materialSerializer;
	};



	/// <summary>
	/// Material reader
	/// <para/> More than one can exist at a time for given material instance, but the existance can not overlap with a Writer; 
	/// Material::GetFields creates a writer under the hood, so be wary of that too.
	/// </summary>
	class JIMARA_API Material::Reader final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="material"> Material to read </param>
		inline Reader(const Material* material)
			: m_lock([&]() -> std::shared_mutex& { static thread_local std::shared_mutex mut; return material == nullptr ? mut : material->m_readWriteLock; }())
			, m_material(material) {}

		/// <summary> Destructor </summary>
		inline virtual ~Reader() {}

		/// <summary> Lit-shader, associated with the material </summary>
		inline const LitShader* Shader()const {
			return (m_material == nullptr) ? nullptr : m_material->m_shader.operator->();
		}

		/// <summary>
		/// Retrieves MaterialProperty field
		/// </summary>
		/// <typeparam name="ValueType"> Field type </typeparam>
		/// <param name="materialPropertyName"> Field name as defined in .jls file </param>
		/// <returns> Field value </returns>
		template<typename ValueType>
		inline ValueType GetPropertyValue(const std::string_view& materialPropertyName)const {
			return Material::GetPropertyValue<ValueType>(m_material, materialPropertyName);
		}

		/// <summary> 
		/// Creates a new Instance that stores current snapshot of the material;
		/// <para/> Instance created this way will not keep track of the source material and will not stay up-to-date upon ANY alteration.
		/// </summary>
		Reference<Instance> CreateSnapshot()const;

		/// <summary>
		/// Shared instance of the material 
		/// <para/> Always up to date with the bindings and shader class; will change automatically as long as material shader stays intact;
		/// <para/> If material shader gets altered, OnInvalidateSharedInstance will be invoked and a new shared instance will be created upon request, 
		/// while the old one will simply keep the last snapshot before invalidation.
		/// <para/> Since material fields can change any time, it's not safe to use shared instance directly as a source of render-time bindings; 
		/// use hand-managed cached instance of it for that purpose.
		/// </summary>
		Reference<const Instance> SharedInstance()const;

	private:
		// Read-lock
		const std::shared_lock<std::shared_mutex> m_lock;

		// Material
		const Reference<const Material> m_material;
	};

	

	/// <summary>
	/// Material writer 
	/// <para/> Only one can exist at a time for given material instance; 
	/// Material::GetFields creates a writer under the hood, so be wary of that too.
	/// </summary>
	class JIMARA_API Material::Writer final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="material"> Material to edit </param>
		Writer(Material* material);

		/// <summary> Destructor </summary>
		virtual ~Writer();

		/// <summary> Lit-shader, associated with the material </summary>
		inline const LitShader* Shader()const { 
			return (m_material == nullptr) ? nullptr : m_material->m_shader.operator->(); 
		}

		/// <summary>
		/// Sets lit-shader of the material
		/// <para/> If set to nullptr, material will NOT render at all...
		/// </summary>
		/// <param name="shader"> Lit shader to use </param>
		void SetShader(const LitShader* shader);

		/// <summary>
		/// Retrieves MaterialProperty field
		/// </summary>
		/// <typeparam name="ValueType"> Field type </typeparam>
		/// <param name="materialPropertyName"> Field name as defined in .jls file </param>
		/// <returns> Field value </returns>
		template<typename ValueType>
		inline ValueType GetPropertyValue(const std::string_view& materialPropertyName)const { 
			return Material::GetPropertyValue<ValueType>(m_material, materialPropertyName);
		}

		/// <summary>
		/// Sets MaterialProperty field value
		/// </summary>
		/// <typeparam name="ValueType"> Field type </typeparam>
		/// <param name="materialPropertyName"> Field name as defined in .jls file </param>
		/// <param name="value"> Value to set </param>
		template<typename ValueType>
		inline void SetPropertyValue(const std::string_view& materialPropertyName, const ValueType& value) { 
			if (Material::SetPropertyValue<ValueType>(m_material, materialPropertyName, value))
				m_flags |= FLAG_FIELDS_DIRTY;
		}

		/// <summary>
		/// Serializes stored material by exposing it's underlying properties
		/// </summary>
		/// <param name="shaderSelector"> Lit-shader selector </param>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		void GetFields(const LitShaderSerializer* shaderSelector, const Callback<Serialization::SerializedObject>& recordElement);

	private:
		// Material
		const Reference<Material> m_material;

		// Modification flags
		static const constexpr uint8_t FLAG_FIELDS_DIRTY = 1;
		static const constexpr uint8_t FLAG_SHADER_DIRTY = 2;
		uint8_t m_flags = 0;
	};



	/// <summary>
	/// Material instance (this one, basically, has a fixed set of the lit shader instance and the available resource bindings)
	/// <para/> Note: Shader bindings are automatically updated for shared Instance, when underlying material changes their values, 
	/// but new resources are not added or removed dynamically.
	/// <para/> Conversely, CachedInstance inherits Instance, but it needs Update() call to be up to date with the base instance.
	/// <para/> Since it's impossible to predict when binding content of the shared instance will change, it's bindings SHOULD NOT be used during rendering;
	/// To circumvent that limitation, one should create a cached instance of it and update it on Graphics sync point, when render jobs are not running.
	/// </summary>
	class JIMARA_API Material::Instance : public virtual Resource {
	public:
		/// <summary> Destructor </summary>
		virtual ~Instance();

		/// <summary> Lit-shader used by instance </summary>
		inline const LitShader* Shader()const { return m_shader; }

		/// <summary> Binding for the settings constant buffer </summary>
		inline const Graphics::ResourceBinding<Graphics::Buffer>* SettingsCBufferBinding()const { return m_settingsConstantBuffer; }

		/// <summary> Bindless-Id of the structured settings buffer </summary>
		inline const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* SettingsBufferBindlessId()const { return m_settingsBufferId; }

		/// <summary>
		/// Returns SettingsCBufferBinding binding, if bindingName corresponds to the shader binding name as defined by the preprocessor (ei SETTINGS_BUFFER_BINDING_NAME)
		/// </summary>
		/// <param name="bindingName"> CBuffer binding name within the shader, as defined by the preprocessor </param>
		/// <returns> SettingsCBufferBinding if bindingName is SETTINGS_BUFFER_BINDING_NAME; nullptr otherwise </returns>
		inline const Graphics::ResourceBinding<Graphics::Buffer>* FindConstantBufferBinding(const std::string_view& bindingName)const {
			return (bindingName == SETTINGS_BUFFER_BINDING_NAME) ? m_settingsConstantBuffer.operator->() : nullptr;
		}

		/// <summary>
		/// Searches for a texture sampler binding by shader binding name (not property name!)
		/// </summary>
		/// <param name="bindingName"> Sampler binding name within the shader, as defined by the preprocessor </param>
		/// <returns> Sampler binding if found, nullptr otherwise </returns>
		const Graphics::ResourceBinding<Graphics::TextureSampler>* FindTextureSamplerBinding(const std::string_view& bindingName)const;

		/// <summary> Generates binding search functions for the material instance </summary>
		Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const;

		/// <summary>
		/// Creates a cached-instance based on this Instance
		/// <para/> Since it's impossible to predict when binding content of the shared instance will change, it's bindings SHOULD NOT be used during rendering;
		/// To circumvent that limitation, one should create a cached instance of it and update it on Graphics sync point, when render jobs are not running.
		/// </summary>
		Reference<CachedInstance> CreateCachedInstance()const;

	private:
		// Lit-Shader
		const Reference<const LitShader> m_shader;

		// Settings buffer
		const Reference<Graphics::ResourceBinding<Graphics::Buffer>> m_settingsConstantBuffer =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>();
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_settingsBufferId;

		// Image bindings
		struct JIMARA_API ImageBinding : public virtual Graphics::ResourceBinding<Graphics::TextureSampler> {
			inline virtual ~ImageBinding() {}
			std::string bindingName;
			Reference<const Graphics::BindlessSet<Graphics::TextureSampler>::Binding> samplerId;
		};
		Stacktor<Reference<ImageBinding>, 4u> m_imageBindings;

		// Lock for the data (used only for copy-operations; binding access is not protected)
		mutable std::shared_mutex m_dataLock;

		// Material can copy it's contents to an instance, as well as create the shared instance and snapshots
		friend class Material;
		void CopyFrom(const Material* material);
		Instance(const LitShader* shader);

		// Cached instance needs access to internals
		friend class CachedInstance;
	};
	
	
	
	/// <summary>
	/// Cached instance of a material 
	/// <para/> Update() call is requred to update binding values;
	/// <para/> Since it's impossible to predict when binding content of the shared instance will change, it's bindings SHOULD NOT be used during rendering;
	/// To circumvent that limitation, one should create a cached instance of it and update it on Graphics sync point, when render jobs are not running.
	/// </summary>
	class JIMARA_API Material::CachedInstance : public virtual Instance {
	public:
		/// <summary> Virtual destructor </summary>
		virtual ~CachedInstance();

		/// <summary> Updates bindings </summary>
		void Update();

	private:
		// Base instance
		const Reference<const Instance> m_baseInstance;

		// CachedInstance can only be created from an existing Instance
		friend class Instance;
		CachedInstance(const Instance* base);
	};


	/// <summary> Serializer for Property </summary>
	struct Material::Property::Serializer : public virtual Serialization::SerializerList::From<Property> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the GUID field </param>
		/// <param name="hint"> Feild description </param>
		/// <param name="attributes"> Item serializer attribute list </param>
		Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {});

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Property* target)const final override;
	};

	/// <summary> Serializer for EditorPath </summary>
	struct Material::EditorPath::Serializer : public virtual Serialization::SerializerList::From<EditorPath> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the GUID field </param>
		/// <param name="hint"> Feild description </param>
		/// <param name="attributes"> Item serializer attribute list </param>
		Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {});

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, EditorPath* target)const final override;
	};

	/// <summary> Default serializer for a material </summary>
	class Material::Serializer : public virtual Serialization::SerializerList::From<Material> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shaderSelector"> Shader selector </param>
		/// <param name="name"> Name of the GUID field </param>
		/// <param name="hint"> Feild description </param>
		/// <param name="attributes"> Item serializer attribute list </param>
		Serializer(
			const LitShaderSerializer* shaderSelector, 
			const std::string_view& name, const std::string_view& hint = "", 
			const std::vector<Reference<const Object>>& attributes = {});

		/// <summary>
		/// Constructor (no shader selection)
		/// </summary>
		/// <param name="name"> Name of the GUID field </param>
		/// <param name="hint"> Feild description </param>
		/// <param name="attributes"> Item serializer attribute list </param>
		Serializer(
			const std::string_view& name, const std::string_view& hint = "", 
			const std::vector<Reference<const Object>>& attributes = {});

		/// <summary> Virtual destructor </summary>
		virtual ~Serializer();

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Material* target)const override;

	private:
		// Shader selector
		const Reference<const LitShaderSerializer> m_shaderSelector;
	};





	template<typename ValueType>
	inline ValueType Material::GetPropertyValue(const Material* material, const std::string_view& materialPropertyName) {
		if (material == nullptr || material->m_shader == nullptr)
			return ValueType(0);
		size_t propertyId = material->m_shader->PropertyIdByName(materialPropertyName);
		if (propertyId == NO_ID) 
			return ValueType(0);
		const PropertyInfo& property = material->m_shader->Property(propertyId);
		if (property.type == PropertyType::SAMPLER2D) {
			const constexpr bool isSamplerReferenceType = std::is_same_v<ValueType, Graphics::TextureSampler*> || std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>;
			if (isSamplerReferenceType) {
				static_assert(sizeof(size_t) == sizeof(Graphics::TextureSampler*));
				const auto it = material->m_imageByBindingName.find(property.bindingName);
				assert(it != material->m_imageByBindingName.end());
				if (it->second->isDefault)
					return ValueType(0);
				Graphics::TextureSampler* const samplerAddress = static_cast<Graphics::TextureSampler*>(it->second->samplerBinding->BoundObject());
				static_assert(std::is_same_v<std::conditional_t<sizeof(uint32_t) < sizeof(uint64_t), uint32_t, size_t>, uint32_t>);
				const auto ptrVal = static_cast<std::conditional_t<isSamplerReferenceType, size_t, float>>((size_t)samplerAddress);
				return ValueType(ptrVal);
			}
			else {
				material->GraphicsDevice()->Log()->Error(
					"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a texture sampler! [", __FILE__, "; ", __LINE__, "]");
				return ValueType(0);
			}
		}
		else if (PropertyTypeId(property.type) != TypeId::Of<ValueType>()) {
			material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a ",
				PropertyTypeId(property.type).Name(), ", not a ", TypeId::Of<ValueType>().Name(), "! [", __FILE__, "; ", __LINE__, "]");
			return ValueType(0);
		}
		else return *reinterpret_cast<const ValueType*>(material->m_settingsBufferData.Data() + property.settingsBufferOffset);
	}

	template<typename ValueType>
	inline bool Material::SetPropertyValue(Material* material, const std::string_view& materialPropertyName, const ValueType& value) {
		if (material == nullptr || material->m_shader == nullptr)
			return false;
		size_t propertyId = material->m_shader->PropertyIdByName(materialPropertyName);
		if (propertyId == NO_ID)
			return false;
		const PropertyInfo& property = material->m_shader->Property(propertyId);
		if (property.type == PropertyType::SAMPLER2D) {
			if (std::is_same_v<ValueType, Graphics::TextureSampler*> || std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>) {
				static_assert(sizeof(size_t) == sizeof(Graphics::TextureSampler*));
				const auto it = material->m_imageByBindingName.find(property.bindingName);
				assert(it != material->m_imageByBindingName.end());
				Graphics::TextureSampler* samplerValue;
				const ValueType* val = &value;
				if (std::is_same_v<ValueType, Graphics::TextureSampler*>)
					samplerValue = (*reinterpret_cast<Graphics::TextureSampler* const *>(val));
				else if (std::is_same_v<ValueType, Reference<Graphics::TextureSampler>>)
					samplerValue = (*reinterpret_cast<const Reference<Graphics::TextureSampler>*>(val));
				else samplerValue = nullptr;
				if ((it->second->isDefault) && it->second->samplerBinding != nullptr && it->second->samplerBinding->BoundObject() == samplerValue)
					return false;
				else if (samplerValue == nullptr) {
					it->second->isDefault = true;
					it->second->samplerBinding = Graphics::SharedTextureSamplerBinding(property.defaultValue.vec4, material->GraphicsDevice());
				}
				else {
					it->second->isDefault = false;
					it->second->samplerBinding = Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(samplerValue);
				}
				samplerValue = it->second->samplerBinding->BoundObject();
				if (it->second->samplerId != nullptr && it->second->samplerId->BoundObject() == samplerValue)
					return false;
				it->second->samplerId = material->m_bindlessSamplers->GetBinding(samplerValue);
				assert(it->second->samplerId != nullptr);
				(*reinterpret_cast<uint32_t*>(material->m_settingsBufferData.Data() + property.settingsBufferOffset)) = it->second->samplerId->Index();
				return true;
			}
			else material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a texture sampler! [", __FILE__, "; ", __LINE__, "]");
		}
		else if (PropertyTypeId(property.type) != TypeId::Of<ValueType>())
			material->GraphicsDevice()->Log()->Error(
				"Material::Writer::GetPropertyValue - Type mismatch! '", materialPropertyName, "' is a ",
				PropertyTypeId(property.type).Name(), ", not a ", TypeId::Of<ValueType>().Name(), "! [", __FILE__, "; ", __LINE__, "]");
		else {
			ValueType& curVal = (*reinterpret_cast<ValueType*>(material->m_settingsBufferData.Data() + property.settingsBufferOffset));
			if (curVal == value)
				return false;
			curVal = value;
			return true;
		}
		return false;
	}
}
