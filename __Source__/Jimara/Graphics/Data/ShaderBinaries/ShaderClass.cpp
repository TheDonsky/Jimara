#include "ShaderClass.h"
#include "../../../Data/Serialization/Attributes/EnumAttribute.h"

namespace Jimara {
	namespace Graphics {
		ShaderClass::ShaderClass(const OS::Path& shaderPath) : m_shaderPath(shaderPath), m_pathStr(shaderPath) {}

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
			std::set<Reference<const ShaderClass>> shaders;
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

		size_t ShaderClass::Set::IndexOf(const ShaderClass* shaderClass)const {
			IndexPerShader::const_iterator it = m_indexPerShader.find(shaderClass);
			if (it == m_indexPerShader.end()) return NoIndex();
			else return it->second;
		}

		ShaderClass::Set::Set(const std::set<Reference<const ShaderClass>>& shaders)
			: m_shaders(shaders.begin(), shaders.end())
			, m_indexPerShader([&]() {
				IndexPerShader index;
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
					index.insert(std::make_pair(it->operator->(), index.size()));
				return index;
				}())
			, m_shadersByPath([&]() {
				ShadersByPath shadersByPath;
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
					const std::string path = it->operator->()->ShaderPath();
					if (path.size() > 0)
						shadersByPath[path] = *it;
				}
				return shadersByPath;
				}())
			, m_classSelector([&]() {
				typedef Serialization::EnumAttribute<std::string_view> EnumAttribute;
				std::vector<EnumAttribute::Choice> choices;
				choices.push_back(EnumAttribute::Choice("<None>", ""));
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
					const std::string& path = it->operator->()->m_pathStr;
					choices.push_back(EnumAttribute::Choice(path, path));
				}
				typedef std::string_view(*GetFn)(const ShaderClass**);
				typedef void(*SetFn)(const std::string_view&, const ShaderClass**);
				return Serialization::ValueSerializer<std::string_view>::For<const ShaderClass*>("Shader", "Shader class", 
					(GetFn)[](const ShaderClass** shaderClass) -> std::string_view {
						if (shaderClass == nullptr || (*shaderClass) == nullptr) return "";
						else return (*shaderClass)->m_pathStr;
					},
					(SetFn)[](const std::string_view& value, const ShaderClass** shaderClass) {
						if (shaderClass == nullptr) return;
						Reference<Set> allShaders = Set::All();
						(*shaderClass) = allShaders->FindByPath(value);
					}, { Object::Instantiate<EnumAttribute>(choices, false) });
				}()){
#ifndef NDEBUG
			for (size_t i = 0; i < m_shaders.size(); i++)
				assert(IndexOf(m_shaders[i]) == i);
#endif // !NDEBUG
		}
	}
}
