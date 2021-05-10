#pragma once
#include "../../GraphicsDevice.h"
#include <string>

namespace Jimara {
	namespace Graphics {
		namespace ShaderResourceBindings {
			/// <summary>
			/// Generic construct for binding resources to pipeline descriptors
			/// </summary>
			/// <typeparam name="ResourceType"> Bound resource type </typeparam>
			template<typename ResourceType>
			class ShaderBinding : public virtual Object {
			private:
				// Binding name within the SPIR-V binary
				const std::string m_bindingName;

				// Bound object
				Reference<ResourceType> m_object;


			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="bindingName"> Binding name within the SPIR-V binary </param>
				/// <param name="object"> Bound object </param>
				ShaderBinding(std::string&& bindingName, ResourceType* object = nullptr) : m_bindingName(std::move(bindingName)), m_object(object) {}

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="bindingName"> Binding name within the SPIR-V binary </param>
				/// <param name="object"> Bound object </param>
				ShaderBinding(const std::string& bindingName, ResourceType* object = nullptr) : ShaderBinding(std::move(std::string(bindingName)), object) {}

				/// <summary> Binding name within the SPIR-V binary </summary>
				const std::string& BindingName()const { return m_bindingName; }

				/// <summary> Bound object (read/write) </summary>
				Reference<ResourceType>& BoundObject() { return m_object; }

				/// <summary> Bound object (read-only) </summary>
				ResourceType* BoundObject()const { return m_object; }
			};

			/// <summary> Resource binding type definition for constant/uniform buffers </summary>
			typedef ShaderBinding<Buffer> ConstantBufferBinding;

			/// <summary> Resource binding type definition for structured buffers </summary>
			typedef ShaderBinding<ArrayBuffer> StructuredBufferBinding;

			/// <summary> Resource binding type definition for texture samplers buffers </summary>
			typedef ShaderBinding<TextureSampler> TextureSamplerBinding;


			/// <summary>
			/// Interface that provides resource bindings by name
			/// </summary>
			class ShaderResourceBindingSet {
			public:
				/// <summary> Virtual destructor </summary>
				virtual inline ~ShaderResourceBindingSet() {}

				/// <summary>
				/// Attempts to find constant buffer binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const ConstantBufferBinding* FindConstantBufferBinding(const std::string& name)const = 0;
				
				/// <summary>
				/// Attempts to find structured buffer binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const StructuredBufferBinding* FindStructuredBufferBinding(const std::string& name)const = 0;

				/// <summary>
				/// Attempts to find texture sampler binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const TextureSamplerBinding* FindTextureSamplerBinding(const std::string& name)const = 0;
			};


			/// <summary>
			/// Simple struct, containing resource bindings, for the simplest ShaderResourceBindingSet implementation 
			/// (not advised to be used for cases with many bound resources for performance considerations; works fine with small sets)
			/// </summary>
			struct ShaderBindingDescription : public virtual ShaderResourceBindingSet {
				/// <summary> Constant buffer bindings </summary>
				const ConstantBufferBinding* const* constantBufferBindings = nullptr;

				/// <summary> Numeber of elements within constantBufferBindings </summary>
				size_t constantBufferBindingCount = 0;
				
				/// <summary> Structured buffer bindings </summary>
				const StructuredBufferBinding* const* structuredBufferBindings = nullptr;

				/// <summary> Numeber of elements within structuredBufferBindings </summary>
				size_t structuredBufferBindingCount = 0;
				
				/// <summary> Texture sampler bindings </summary>
				const TextureSamplerBinding* const* textureSamplerBindings = nullptr;

				/// <summary> Numeber of elements within textureSamplerBindings </summary>
				size_t textureSamplerBindingCount = 0;

				/// <summary>
				/// Attempts to find constant buffer binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const ConstantBufferBinding* FindConstantBufferBinding(const std::string& name)const override;

				/// <summary>
				/// Attempts to find structured buffer binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const StructuredBufferBinding* FindStructuredBufferBinding(const std::string& name)const override;

				/// <summary>
				/// Attempts to find texture sampler binding by name
				/// </summary>
				/// <param name="name"> Binding name </param>
				/// <returns> Binding reference if found, nullptr otherwise </returns>
				virtual const TextureSamplerBinding* FindTextureSamplerBinding(const std::string& name)const override;
			};


			/// <summary> 
			/// PipelineDescriptor::BindingSetDescriptor alingside it's set id:
			/// </summary>
			struct BindingSetInfo {
				Reference<PipelineDescriptor::BindingSetDescriptor> set;
				size_t setIndex;
			};


			/// <summary>
			/// Generates PipelineDescriptor::BindingSetDescriptor objects for given set of shaders (presumably from the same pipeline) given the resource bindings
			/// Note: Ignores sets that have no resources bound to them and automatically thinks of them as something, provided by the environment.
			/// </summary>
			/// <param name="shaderBinaries"> SPIRV-Binaries, the descriptors should be compatible with </param>
			/// <param name="shaderBinaryCount"> Number of elements within shaderBinaries </param>
			/// <param name="bindings"> Resource bindings </param>
			/// <param name="addDescriptor"> Callback, that will receive information about the "discovered" binding sets </param>
			/// <param name="logger"> Logger for logging errors/warnings </param>
			/// <returns> True, if all binding sets were generated sucessfully </returns>
			bool GenerateShaderBindings(
				const SPIRV_Binary* const* shaderBinaries, size_t shaderBinaryCount,
				const ShaderResourceBindingSet& bindings,
				Callback<const BindingSetInfo&> addDescriptor, 
				OS::Logger* logger);

			/// <summary>
			/// Generates PipelineDescriptor::BindingSetDescriptor objects for given set of shaders (presumably from the same pipeline) given the resource bindings
			/// Note: Ignores sets that have no resources bound to them and automatically thinks of them as something, provided by the environment.
			/// </summary>
			/// <typeparam name="CallbackType"> Type of addDescriptor callback (enables lambdas and such) </typeparam>
			/// <param name="shaderBinaries"> SPIRV-Binaries, the descriptors should be compatible with </param>
			/// <param name="shaderBinaryCount"> Number of elements within shaderBinaries </param>
			/// <param name="bindings"> Resource bindings </param>
			/// <param name="addDescriptor"> Callback, that will receive information about the "discovered" binding sets </param>
			/// <param name="logger"> Logger for logging errors/warnings </param>
			/// <returns> True, if all binding sets were generated sucessfully </returns>
			template<typename CallbackType>
			inline static bool GenerateShaderBindings(
				const SPIRV_Binary* const* shaderBinaries, size_t shaderBinaryCount,
				const ShaderResourceBindingSet& bindings,
				const CallbackType& addDescriptor, 
				OS::Logger* logger) {
				static thread_local const CallbackType* addFunction = nullptr;
				static thread_local Callback<const BindingSetInfo&> addCallback([](const BindingSetInfo& info) { (*addFunction)(info); });
				addFunction = &addDescriptor;
				return GenerateShaderBindings(shaderBinaries, shaderBinaryCount, bindings, addCallback, logger);
			}



			/// <summary>
			/// Singular SPIRV_Binary::BindingSetInfo and corresponding shader stages
			/// </summary>
			struct ShaderModuleBindingSet {
				/// <summary> Binding set information </summary>
				const SPIRV_Binary::BindingSetInfo* set;

				/// <summary> Pipeline stages, during which this binding set should be visible </summary>
				PipelineStageMask stages;

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="setInfo"> Binding set information </param>
				/// <param name="stageMask"> Pipeline stages, during which this binding set should be visible </param>
				inline ShaderModuleBindingSet(const SPIRV_Binary::BindingSetInfo* setInfo = nullptr, PipelineStageMask stageMask = 0) : set(setInfo), stages(stageMask) {}
			};

			/// <summary>
			/// Generates PipelineDescriptor::BindingSetDescriptor objects for given set of SPIR-V bindings (presumably from the same pipeline) given the resource bindings
			/// Note: Ignores sets that have no resources bound to them and automatically thinks of them as something, provided by the environment.
			/// </summary>
			/// <param name="binaryBindingSets"> ShaderModuleBindingSet objects, containing SPIR-V binding sets and corresponding pipeline stages </param>
			/// <param name="bindingSetCount"> Number of elements within binaryBindingSets </param>
			/// <param name="bindings"> Resource bindings </param>
			/// <param name="addDescriptor"> Callback, that will receive information about the "discovered" binding sets </param>
			/// <param name="logger"> Logger for logging errors/warnings </param>
			/// <returns> True, if all binding sets were generated sucessfully </returns>
			bool GenerateShaderBindings(
				const ShaderModuleBindingSet* binaryBindingSets, size_t bindingSetCount,
				const ShaderResourceBindingSet& bindings,
				const Callback<const BindingSetInfo&>& addDescriptor,
				OS::Logger* logger);

			/// <summary>
			/// Generates PipelineDescriptor::BindingSetDescriptor objects for given set of SPIR-V bindings (presumably from the same pipeline) given the resource bindings
			/// Note: Ignores sets that have no resources bound to them and automatically thinks of them as something, provided by the environment.
			/// </summary>
			/// <typeparam name="CallbackType"> Type of addDescriptor callback (enables lambdas and such) </typeparam>
			/// <param name="binaryBindingSets"> ShaderModuleBindingSet objects, containing SPIR-V binding sets and corresponding pipeline stages </param>
			/// <param name="bindingSetCount"> Number of elements within binaryBindingSets </param>
			/// <param name="bindings"> Resource bindings </param>
			/// <param name="addDescriptor"> Callback, that will receive information about the "discovered" binding sets </param>
			/// <param name="logger"> Logger for logging errors/warnings </param>
			/// <returns> True, if all binding sets were generated sucessfully </returns>
			template<typename CallbackType>
			inline static bool GenerateShaderBindings(
				const ShaderModuleBindingSet* binaryBindingSets, size_t bindingSetCount,
				const ShaderResourceBindingSet& bindings,
				const CallbackType& addDescriptor,
				OS::Logger* logger) {
				static thread_local const CallbackType* addFunction = nullptr;
				static thread_local Callback<const BindingSetInfo&> addCallback([](const BindingSetInfo& info) { (*addFunction)(info); });
				addFunction = &addDescriptor;
				return GenerateShaderBindings(binaryBindingSets, bindingSetCount, bindings, addCallback, logger);
			}
		}
	}
}
