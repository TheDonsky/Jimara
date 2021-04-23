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
		class SPIRV_Binary : public virtual Object {
		public:
			struct BindingInfo {
				enum class Type : uint8_t {
					UNKNOWN,
					CONSTANT_BUFFER,
					TEXTURE_SAMPLER,
					STRUCTURED_BUFFER
				};

				std::string name;
				size_t set;
				size_t binding;
				Type type;
				size_t index;
			};

			class BindingSetInfo {
			public:
				size_t Id()const;

				size_t BindingCount()const;

				const BindingInfo& Binding(size_t index)const;

				const bool FindBinding(size_t bindingId, const BindingInfo*& binding)const;

				const bool FindBinding(const std::string& bindingName, const BindingInfo*& binding)const;

			private:
				const size_t m_id;
				const std::vector<BindingInfo> m_bindings;
				const std::unordered_map<size_t, size_t> m_idToIndex;
				const std::unordered_map<std::string, size_t> m_nameToIndex;

				BindingSetInfo(size_t id, const std::vector<BindingInfo>& bindings);
				friend class SPIRV_Binary;
			};

			static Reference<SPIRV_Binary> FromSPV(const std::string& filename, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(std::vector<uint8_t>&& data, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const void* data, size_t size, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const std::vector<uint8_t>& data, OS::Logger* logger);

			static Reference<SPIRV_Binary> FromData(const std::vector<char>& data, OS::Logger* logger);

			virtual ~SPIRV_Binary();

			const void* Bytecode()const;

			size_t BytecodeSize()const;

			const std::string& EntryPoint()const;

			PipelineStageMask ShaderStages()const;

			size_t BindingSetCount()const;

			const BindingSetInfo& BindingSet(size_t index)const;

			const bool FindBinding(const std::string& bindingName, const BindingInfo*& binding)const;


		private:
			const Reference<OS::Logger> m_logger;

			const std::vector<uint8_t> m_bytecode;

			const std::string m_entryPoint;

			const PipelineStageMask m_stageMask;

			const std::vector<BindingSetInfo> m_bindingSets;

			std::unordered_map<std::string, std::pair<size_t, size_t>> m_bindingNameToSetIndex;

			SPIRV_Binary(
				std::vector<uint8_t>&& bytecode,
				std::string&& entryPoint,
				PipelineStageMask stageMask,
				std::vector<BindingSetInfo>&& bindingSets,
				OS::Logger* logger);
		};
	}
}
