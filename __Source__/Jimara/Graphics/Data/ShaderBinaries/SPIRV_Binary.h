#pragma once
#include "../../../OS/Logging/Logger.h"
namespace Jimara {
	namespace Graphics {
		class SPIRV_Binary;
	}
}
#include "../../GraphicsDevice.h"
#include <unordered_map>
#include <vector>
#include <string>


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Wrapper around a SPIR-V shader bytecode
		/// </summary>
		class SPIRV_Binary : public virtual Object {
		public:
			/// <summary>
			/// Information about a single shader binding
			/// </summary>
			struct BindingInfo {
				/// <summary>
				/// Binding type
				/// </summary>
				enum class Type : uint8_t {
					/// <summary> Unknown/Unsupported type </summary>
					UNKNOWN,

					/// <summary> Constant/Uniform buffer </summary>
					CONSTANT_BUFFER,

					/// <summary> Texture sampler </summary>
					TEXTURE_SAMPLER,

					/// <summary> Structured/Storage buffer </summary>
					STRUCTURED_BUFFER
				};

				/// <summary> Name of the binding </summary>
				std::string name;

				/// <summary> Binding set id </summary>
				size_t set = 0;

				/// <summary> Binding id </summary>
				size_t binding = 0;

				/// <summary> Binding type </summary>
				Type type = Type::UNKNOWN;

				/// <summary> Binding index within BindingSetInfo </summary>
				size_t index = 0;
			};

			/// <summary>
			/// Information about a binding set
			/// </summary>
			class BindingSetInfo {
			public:
				/// <summary> Binding set id </summary>
				size_t Id()const;

				/// <summary> Number of bindings within the set </summary>
				size_t BindingCount()const;

				/// <summary>
				/// Binding by index
				/// </summary>
				/// <param name="index"> Binding index (not the same as binding id) </param>
				/// <returns> Binding information </returns>
				const BindingInfo& Binding(size_t index)const;

				/// <summary>
				/// Searches for the binding with binding id
				/// </summary>
				/// <param name="bindingId"> Binding id </param>
				/// <param name="binding"> Reference to store binding address at </param>
				/// <returns> True, if found </returns>
				const bool FindBinding(size_t bindingId, const BindingInfo*& binding)const;

				/// <summary>
				/// Searches for the binding with binding name
				/// </summary>
				/// <param name="bindingName"> Binding name </param>
				/// <param name="binding"> Reference to store binding address at </param>
				/// <returns> True, if found </returns>
				const bool FindBinding(const std::string& bindingName, const BindingInfo*& binding)const;

			private:
				// Set id
				const size_t m_id;

				// Set bindings
				const std::vector<BindingInfo> m_bindings;

				// Binding id to binding index mapping
				const std::unordered_map<size_t, size_t> m_idToIndex;

				// Binding name to binding index mapping
				const std::unordered_map<std::string, size_t> m_nameToIndex;

				// Constructor
				BindingSetInfo(size_t id, const std::vector<BindingInfo>& bindings);

				// Only SPIRV_Binary can call the constructor
				friend class SPIRV_Binary;
			};

			/// <summary>
			/// Reads SPIR-V binary from .spv file
			/// </summary>
			/// <param name="filename"> File to read from </param>
			/// <param name="logger"> Logger </param>
			/// <returns> SPIRV_Binary instance if found, nullptr otherwise </returns>
			static Reference<SPIRV_Binary> FromSPV(const std::string& filename, OS::Logger* logger);

			/// <summary>
			/// Makes a wrapper around a SPIR-V binary
			/// </summary>
			/// <param name="data"> SPIR-V binary data </param>
			/// <param name="logger"> Logger </param>
			/// <returns> SPIRV_Binary instance if data is valid, nullptr otherwise </returns>
			static Reference<SPIRV_Binary> FromData(std::vector<uint8_t>&& data, OS::Logger* logger);

			/// <summary>
			/// Makes a wrapper around a SPIR-V binary
			/// </summary>
			/// <param name="data"> SPIR-V binary data </param>
			/// <param name="size"> Data size in bytes </param>
			/// <param name="logger"> Logger </param>
			/// <returns> SPIRV_Binary instance if data is valid, nullptr otherwise </returns>
			static Reference<SPIRV_Binary> FromData(const void* data, size_t size, OS::Logger* logger);

			/// <summary>
			/// Makes a wrapper around a SPIR-V binary
			/// </summary>
			/// <param name="data"> SPIR-V binary data </param>
			/// <param name="logger"> Logger </param>
			/// <returns> SPIRV_Binary instance if data is valid, nullptr otherwise </returns>
			static Reference<SPIRV_Binary> FromData(const std::vector<uint8_t>& data, OS::Logger* logger);

			/// <summary>
			/// Makes a wrapper around a SPIR-V binary
			/// </summary>
			/// <param name="data"> SPIR-V binary data </param>
			/// <param name="logger"> Logger </param>
			/// <returns> SPIRV_Binary instance if data is valid, nullptr otherwise </returns>
			static Reference<SPIRV_Binary> FromData(const std::vector<char>& data, OS::Logger* logger);

			/// <summary> Virtual destructor </summary>
			virtual ~SPIRV_Binary();

			/// <summary> SPIR-V bytecode data </summary>
			const void* Bytecode()const;

			/// <summary> SPIR-V bytecode data size in bytes </summary>
			size_t BytecodeSize()const;

			/// <summary> Shader entry point </summary>
			const std::string& EntryPoint()const;

			/// <summary> Shader stages, the code is applicable to </summary>
			PipelineStageMask ShaderStages()const;

			/// <summary> Number of shader binding sets </summary>
			size_t BindingSetCount()const;

			/// <summary>
			/// Shader binding set by index
			/// </summary>
			/// <param name="index"> Set index (not the same as set id) </param>
			/// <returns> Binding set </returns>
			const BindingSetInfo& BindingSet(size_t index)const;

			/// <summary>
			/// Searches for a binding by name
			/// </summary>
			/// <param name="bindingName"> Binding name </param>
			/// <param name="binding"> Reference to store binding address at </param>
			/// <returns> True, if found </returns>
			const bool FindBinding(const std::string& bindingName, const BindingInfo*& binding)const;


		private:
			// Logger
			const Reference<OS::Logger> m_logger;

			// Bytecode data
			const std::vector<uint8_t> m_bytecode;

			// Entry point
			const std::string m_entryPoint;

			// Shader stages, the code is applicable to
			const PipelineStageMask m_stageMask;

			// Binding sets
			const std::vector<BindingSetInfo> m_bindingSets;

			// Binding index
			std::unordered_map<std::string, std::pair<size_t, size_t>> m_bindingNameToSetIndex;

			// Constructor
			SPIRV_Binary(
				std::vector<uint8_t>&& bytecode,
				std::string&& entryPoint,
				PipelineStageMask stageMask,
				std::vector<BindingSetInfo>&& bindingSets,
				OS::Logger* logger);
		};
	}
}
