#include "ShaderClass.h"

namespace Jimara {
	namespace Graphics {
		ShaderClass::ShaderClass(const OS::Path& shaderPath) : m_shaderPath(shaderPath) {}

		const OS::Path& ShaderClass::ShaderPath()const { return m_shaderPath; }

		Reference<ShaderClass::Set> ShaderClass::Set::All() {
			static SpinLock allLock;
			static Reference<Set> all;
			static Reference<RegisteredTypeSet> registeredTypes;

			std::unique_lock<SpinLock> lock(allLock);
			{
				const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
				if (currentTypes == registeredTypes) return all;
				else registeredTypes = currentTypes;
			}
			std::unordered_set<Reference<const ShaderClass>> shaders;
			for (size_t i = 0; i < registeredTypes->Size(); i++) {
				void(*checkAttribute)(decltype(shaders)*, const Object*) = [](decltype(shaders)* shaderSet, const Object* attribute) {
					const ShaderClass* shaderClass = dynamic_cast<const ShaderClass*>(attribute);
					if (shaderClass != nullptr)
						shaderSet->insert(shaderClass);
				};
				registeredTypes->At(i).GetAttributes(Callback<const Object*>(checkAttribute, &shaders));
			}
			all = new Set(shaders);
			all->ReleaseRef();
			return all;
		}

		size_t ShaderClass::Set::Size()const { return m_shaders.size(); }

		const ShaderClass* ShaderClass::Set::At(size_t index)const { return m_shaders[index]; }

		const ShaderClass* ShaderClass::Set::operator[](size_t index)const { return At(index); }

		const ShaderClass* ShaderClass::Set::FindByPath(const OS::Path& shaderPath)const {
			const decltype(m_shadersByPath)::const_iterator it = m_shadersByPath.find(shaderPath);
			if (it == m_shadersByPath.end()) return nullptr;
			else return it->second;
		}

		ShaderClass::Set::Set(const std::unordered_set<Reference<const ShaderClass>>& shaders)
			: m_shaders(shaders.begin(), shaders.end())
			, m_shadersByPath([&]() {
			ShadersByPath shadersByPath;
			for (std::unordered_set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
				shadersByPath[it->operator->()->ShaderPath()] = *it;
			return shadersByPath;
				}()) {}
	}
}
