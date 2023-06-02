#pragma once
#include "../../../Data/Serialization/ItemSerializers.h"
#include "../../../Core/Helpers.h"
#include "../../../OS/IO/Path.h"
#include "../../GraphicsDevice.h"
#include <set>



namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader class description that helps with shader binary loading, default bindings ans similar stuff
		/// <para /> Note: 
		///		<para />To register a new/custom ShaderClass object, 
		///		<para />just register type with:
		///			<code>JIMARA_REGISTER_TYPE(Namespace::ShaderClassType); </code>
		///		<para /> and report singleton instance with TypeIdDetails: 
		///			<code> <para /> namespace Jimara {
		///				<para /> template&lt;&gt; inline void TypeIdDetails::GetTypeAttributesOf&lt;Namespace::ShaderClassType&gt;(const Callback&lt;const Object*&gt;&#38; report) {
		///					<para /> static const Namespace::ShaderClassType instance; // You can create singleton in any way you wish, btw...
		///					<para /> report(&#38;instance);
		///				<para /> }
		///			<para /> } </code>
		///		<para /> Doing these two sets will cause your class instance to appear in ShaderClass::Set::All(), meaning, it'll be accessible to editor and serializers
		/// </summary>
		class JIMARA_API ShaderClass : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="shaderPath"> Shader path within the project directory </param>
			/// <param name="blendMode"> Graphics blend mode (irrelevant for compute shaders) </param>
			ShaderClass(const OS::Path& shaderPath, GraphicsPipeline::BlendMode blendMode = GraphicsPipeline::BlendMode::REPLACE);

			/// <summary> Shader path within the project directory </summary>
			const OS::Path& ShaderPath()const;

			/// <summary> Graphics blend mode (irrelevant for compute shaders) </summary>
			GraphicsPipeline::BlendMode BlendMode()const;

			/// <summary> Short for ShaderResourceBindings::ConstantBufferBinding </summary>
			typedef ResourceBinding<Buffer> ConstantBufferBinding;

			/// <summary> Short for ShaderResourceBindings::StructuredBufferBinding </summary>
			typedef ResourceBinding<ArrayBuffer> StructuredBufferBinding;

			/// <summary> Short for ShaderResourceBindings::TextureSamplerBinding </summary>
			typedef ResourceBinding<TextureSampler> TextureSamplerBinding;

			/// <summary> Descriptor of a shader class binding set (could be something like a material writer) </summary>
			struct Bindings;

			/// <summary> Set of ShaderClass objects </summary>
			class Set;

			/// <summary>
			/// "Shared" instance of a constant ConstantBufferBinding that has a fixed content
			/// <para /> Notes: 
			///		<para /> 0. Useful for DefaultConstantBufferBinding() implementation;
			///		<para /> 1. If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
			/// </summary>
			/// <param name="bufferData"> Buffer content </param>
			/// <param name="bufferSize"> Content size </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Shared ConstantBufferBinding instance </returns>
			static Reference<const ConstantBufferBinding> SharedConstantBufferBinding(const void* bufferData, size_t bufferSize, GraphicsDevice* device);

			/// <summary>
			/// "Shared" instance of a constant ConstantBufferBinding that has a fixed content
			/// <para /> Notes: 
			///		<para /> 0. Useful for DefaultConstantBufferBinding() implementation;
			///		<para /> 1. If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
			/// </summary>
			/// <param name="content"> Buffer content </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Shared ConstantBufferBinding instance </returns>
			template<typename BufferType>
			inline static Reference<const ConstantBufferBinding> SharedConstantBufferBinding(const BufferType& content, GraphicsDevice* device) {
				return SharedConstantBufferBinding(&content, sizeof(BufferType), device);
			}

			/// <summary>
			/// "Shared" instance of a constant TextureSamplerBinding that binds to a single pixel texture with given color
			/// <para /> Notes: 
			///		<para /> 0. Useful for DefaultTextureSamplerBinding() implementation;
			///		<para /> 1. If user modifies the contents, system will have no way to know and that would be bad. So do not do that crap!
			/// </summary>
			/// <param name="color"> Texture color </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Shared TextureSamplerBinding instance </returns>
			static Reference<const TextureSamplerBinding> SharedTextureSamplerBinding(const Vector4& color, GraphicsDevice* device);


			/// <summary>
			/// Gets default constant buffer binding per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Constant buffer binding instance for the device </returns>
			virtual inline Reference<const ConstantBufferBinding> DefaultConstantBufferBinding(const std::string_view& name, GraphicsDevice* device)const { 
				Unused(name, device); return nullptr; 
			}

			/// <summary>
			/// Gets default structured buffer binding per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Structured buffer binding instance for the device </returns>
			virtual inline Reference<const StructuredBufferBinding> DefaultStructuredBufferBinding(const std::string_view& name, GraphicsDevice* device)const {
				Unused(name, device); return nullptr; 
			}
			
			/// <summary>
			/// Gets default texture per device
			/// </summary>
			/// <param name="name"> Binding name </param>
			/// <param name="device"> Graphics device </param>
			/// <returns> Texture binding instance for the device </returns>
			virtual inline Reference<const TextureSamplerBinding> DefaultTextureSamplerBinding(const std::string_view& name, GraphicsDevice* device)const { 
				Unused(device); 
				return SharedTextureSamplerBinding(Vector4(1.0f), device);
			}

			/// <summary>
			/// Serializes shader bindings (like textures and constants)
			/// </summary>
			/// <param name="reportField"> 
			/// Each binding can be represented as an arbitrary SerializedObject (possibly with some nested entries); 
			/// Serialization scheme can use these to control binding set.
			/// </param>
			/// <param name="bindings"> Binding set to serialize </param>
			virtual inline void SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const { Unused(reportField, bindings); }


			/// <summary>
			/// Helper for serializing constant buffers
			/// </summary>
			/// <typeparam name="BufferType"> Type of the constant buffer content </typeparam>
			template<typename BufferType>
			class ConstantBufferSerializer;

			/// <summary>
			/// Helper for serializing texture sampler bindings
			/// </summary>
			class TextureSamplerSerializer;


		private:
			// Shader path within the project directory
			const OS::Path m_shaderPath;

			// Shader path as string;
			const std::string m_pathStr;

			// Blend mode for graphics pipelines
			const GraphicsPipeline::BlendMode m_blendMode;
		};

		/// <summary> Descriptor of a shader class binding set (could be something like a material writer) </summary>
		struct ShaderClass::Bindings {
			/// <summary> Graphics device, binding set is tied to </summary>
			virtual inline Graphics::GraphicsDevice* GraphicsDevice()const = 0;

			/// <summary>
			/// Constant buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <returns> Constant buffer binding if found, nullptr otherwise </returns>
			virtual Buffer* GetConstantBuffer(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates constant buffer binding
			/// </summary>
			/// <param name="name"> Name of the constant buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetConstantBuffer(const std::string_view& name, Buffer* buffer) = 0;

			/// <summary>
			/// Structured buffer binding by name
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <returns> Structured buffer binding if found, nullptr otherwise </returns>
			virtual ArrayBuffer* GetStructuredBuffer(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates structured buffer binding
			/// </summary>
			/// <param name="name"> Name of the structured buffer binding </param>
			/// <param name="buffer"> New buffer to set (nullptr removes the binding) </param>
			virtual void SetStructuredBuffer(const std::string_view& name, ArrayBuffer* buffer) = 0;

			/// <summary>
			/// Texture sampler binding by name
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <returns> Texture sampler binding if found, nullptr otherwise </returns>
			virtual TextureSampler* GetTextureSampler(const std::string_view& name)const = 0;

			/// <summary>
			/// Updates texture sampler binding
			/// </summary>
			/// <param name="name"> Name of the texture sampler binding </param>
			/// <param name="buffer"> New sampler to set (nullptr removes the binding) </param>
			virtual void SetTextureSampler(const std::string_view& name, TextureSampler* sampler) = 0;
		};

		/// <summary>
		/// Set of ShaderClass objects
		/// </summary>
		class ShaderClass::Set : public virtual Object {
		public:
			/// <summary>
			///  Set of all currently registered ShaderClass objects
			/// Notes: 
			///		0. Value will automagically change whenever any new type gets registered or unregistered;
			///		1. ShaderClass::Set is immutable, so no worries about anything going out of scope or deadlocking.
			/// </summary>
			/// <returns> Reference to current set of all </returns>
			static Reference<Set> All();

			/// <summary> Number of shaders within the set </summary>
			size_t Size()const;

			/// <summary>
			/// ShaderClass by index
			/// </summary>
			/// <param name="index"> ShaderClass index </param>
			/// <returns> ShaderClass </returns>
			const ShaderClass* At(size_t index)const;

			/// <summary>
			/// ShaderClass by index
			/// </summary>
			/// <param name="index"> ShaderClass index </param>
			/// <returns> ShaderClass </returns>
			const ShaderClass* operator[](size_t index)const;

			/// <summary>
			/// Finds ShaderClass by it's ShaderPath() value
			/// </summary>
			/// <param name="shaderPath"> Shader path within the code </param>
			/// <returns> ShaderClass s, such that s->ShaderPath() is shaderPath if found, nullptr otherwise </returns>
			const ShaderClass* FindByPath(const OS::Path& shaderPath)const;

			/// <summary> 'Invalid' shader class index </summary>
			inline static size_t NoIndex() { return ~((size_t)0); }

			/// <summary>
			/// Finds index of a shader class
			/// </summary>
			/// <param name="shaderClass"> Shader class </param>
			/// <returns> Index i, such that this->At(i) is shaderClass; NoIndex if not found </returns>
			size_t IndexOf(const ShaderClass* shaderClass)const;

			/// <summary> Shader class selector </summary>
			typedef Serialization::ItemSerializer::Of<const ShaderClass*> ShaderClassSerializer;

			/// <summary> Shader class selector (Alive and valid only while this set exists..) </summary>
			const ShaderClassSerializer* ShaderClassSelector()const { return m_classSelector; }

		private:
			// List of all known shaders
			const std::vector<Reference<const ShaderClass>> m_shaders;

			// Index of each shader
			typedef std::unordered_map<const ShaderClass*, size_t> IndexPerShader;
			const IndexPerShader m_indexPerShader;

			// Mapping from ShaderClass::ShaderPath() to corresponding ShaderClass()
			typedef std::unordered_map<OS::Path, const ShaderClass*> ShadersByPath;
			const ShadersByPath m_shadersByPath;

			// Shader class selector
			const Reference<const ShaderClassSerializer> m_classSelector;

			// Constructor
			Set(const std::set<Reference<const ShaderClass>>& shaders);
		};

		/// <summary>
		/// Helper for serializing constant buffers
		/// </summary>
		/// <typeparam name="BufferType"> Type of the constant buffer content </typeparam>
		template<typename BufferType>
		class ShaderClass::ConstantBufferSerializer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="bindingName"> Name of the constrant buffer binding (Name from the shader) </param>
			/// <param name="bufferSerializer"> Serializer, that handles buffer content </param>
			/// <param name="defaultValue"> Default value to use, when the field is missing </param>
			inline ConstantBufferSerializer(
				const std::string_view& bindingName,
				const Reference<const Serialization::ItemSerializer::Of<BufferType>>& bufferSerializer,
				const BufferType& defaultValue = {})
				: m_bindingName(bindingName), m_serializer(bufferSerializer), m_defaultValue(defaultValue) {}

			/// <summary>
			/// Serializes the constant buffer binding
			/// </summary>
			/// <param name="reportField"> This will be used for data serialization </param>
			/// <param name="bindings"> Binding set descriptor </param>
			inline void Serialize(const Callback<Serialization::SerializedObject>& reportField, Bindings* bindings)const {
				Buffer* buffer = bindings->GetConstantBuffer(m_bindingName);
				BufferReference<BufferType> typedBuffer;
				bool dirty = (buffer == nullptr || buffer->ObjectSize() != sizeof(BufferType));
				if (dirty) {
					typedBuffer = bindings->GraphicsDevice()->CreateConstantBuffer<BufferType>();
					bindings->SetConstantBuffer(m_bindingName, typedBuffer);
				}
				else typedBuffer = buffer;
				BufferType& data = typedBuffer.Map();
				if (dirty) data = m_defaultValue;
				const BufferType initialData = data;
				reportField(m_serializer->Serialize(data));
				dirty |= (std::memcmp((void*)&initialData, (void*)&data, sizeof(BufferType)) != 0);
				typedBuffer->Unmap(dirty);
			}

		private:
			// Name from the shader
			const std::string m_bindingName;

			// Underlying serializer
			const Reference<const Serialization::ItemSerializer::Of<BufferType>> m_serializer;

			// Default value
			const BufferType m_defaultValue;
		};

		/// <summary>
		/// Helper for serializing texture sampler bindings
		/// </summary>
		class ShaderClass::TextureSamplerSerializer {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="bindingName"> Name of the texture sampler binding (Name from the shader) </param>
			/// <param name="serializerName"> Name of the serializer (will be displayed with this inside Editor UI) </param>
			/// <param name="serializerTooltip"> Tooltip for the serializer </param>
			/// <param name="serializerAttributes"> Serializer attributes </param>
			TextureSamplerSerializer(
				const std::string_view& bindingName,
				const std::string_view& serializerName,
				const std::string_view& serializerTooltip = "",
				const std::vector<Reference<const Object>> serializerAttributes = {});

			/// <summary>
			/// Serializes the texture binding
			/// </summary>
			/// <param name="bindings"> Binding set descriptor </param>
			/// <returns> Serialized object </returns>
			inline Serialization::SerializedObject Serialize(Bindings* bindings)const { return m_serializer->Serialize(bindings); }

		private:
			// Name from the shader
			const std::string m_bindingName;

			// Underlying serializer
			Reference<const Serialization::ItemSerializer::Of<Bindings>> m_serializer;
		};
	}

	// Type id details
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ShaderClass::Set>(const Callback<TypeId>& reportParent) { reportParent(TypeId::Of<Object>()); }
}
