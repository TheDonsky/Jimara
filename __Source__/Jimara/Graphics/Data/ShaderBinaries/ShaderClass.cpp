#include "ShaderClass.h"

namespace Jimara {
	namespace Graphics {
		ShaderClass::ShaderClass(const std::string& shaderPath) : m_shaderPath(shaderPath) {}

		const std::string& ShaderClass::ShaderPath()const { return m_shaderPath; }

		namespace {
			class Registry : public virtual ObjectCache<std::string> {
			public:
				template<class CreateFn>
				static Reference<ShaderClass::RegistryEntry> TryCreate(const std::string& shaderId, const CreateFn& create) {
					static Registry registry;
					return registry.GetCachedOrCreate(shaderId, false, create);
				}
			};
		}

		Reference<ShaderClass::RegistryEntry> ShaderClass::RegistryEntry::Create(const std::string& shaderId, Reference<const ShaderClass> shaderClass) {
			bool created = false;
			Reference<ShaderClass::RegistryEntry> instance = Registry::TryCreate(shaderId, [&]()->Reference<ShaderClass::RegistryEntry> {
				created = true;
				Reference<ShaderClass::RegistryEntry> reference = new RegistryEntry(shaderId, shaderClass);
				reference->ReleaseRef();
				return reference; });
			if (created) return instance;
			else return nullptr;
		}

		ShaderClass::RegistryEntry::RegistryEntry(const std::string& shaderId, Reference<const ShaderClass> shaderClass) 
			: m_shaderId(shaderId), m_shaderClass(shaderClass) {}

		ShaderClass::RegistryEntry::~RegistryEntry() {}

		ShaderClass::RegistryEntry* ShaderClass::RegistryEntry::Find(const std::string& shaderId) {
			return Registry::TryCreate(shaderId, [&]()->Reference<ShaderClass::RegistryEntry> { return nullptr; });
		}

		const std::string& ShaderClass::RegistryEntry::ShaderId()const { return m_shaderId; }

		const ShaderClass* ShaderClass::RegistryEntry::Class()const { return m_shaderClass; }
	}
}
