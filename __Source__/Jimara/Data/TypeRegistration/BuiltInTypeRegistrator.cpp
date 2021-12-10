#include "TypeRegistartion.h"
#include "../../__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h"
#include "../../Core/Collections/ObjectCache.h"
#include <shared_mutex>

namespace Jimara {
	namespace {
		inline static std::shared_mutex& TypeId_RegistryLock() {
			static std::shared_mutex lock;
			return lock;
		}

		typedef std::unordered_map<std::type_index, std::pair<TypeId, size_t>> TypeId_Registry;
		inline static TypeId_Registry& TypeId_GlobalRegistry() {
			static TypeId_Registry registry;
			return registry;
		}

		class TypeId_RegistrationToken : public virtual ObjectCache<TypeId>::StoredObject {
		private:
			const TypeId m_typeId;

		public:
			inline TypeId_RegistrationToken(const TypeId& typeId) : m_typeId(typeId) {
				std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
				TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
				if (it == TypeId_GlobalRegistry().end())
					TypeId_GlobalRegistry()[m_typeId.TypeIndex()] = std::make_pair(m_typeId, 1);
				else it->second.second++;
			}

			inline virtual ~TypeId_RegistrationToken() {
				std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
				TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
				if (it == TypeId_GlobalRegistry().end()) return;
				if (it->second.second > 1)
					it->second.second--;
				else TypeId_GlobalRegistry().erase(it);
			}

			class Cache : public virtual ObjectCache<TypeId> {
			public:
				inline static Reference<TypeId_RegistrationToken> GetToken(const TypeId& typeId) {
					static Cache cache;
					return cache.GetCachedOrCreate(typeId, false, [&]() -> Reference<TypeId_RegistrationToken> {
						return Object::Instantiate<TypeId_RegistrationToken>(typeId);
						});
				}
			};
		};
	}

	Reference<Object> TypeId::Register()const { return TypeId_RegistrationToken::Cache::GetToken(*this); }

	bool TypeId::Find(const std::type_info& typeInfo, TypeId& result) {
		std::shared_lock<std::shared_mutex> lock(TypeId_RegistryLock());
		TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(typeInfo);
		if (it == TypeId_GlobalRegistry().end()) return false;
		else {
			result = it->second.first;
			return true;
		}
	}

	template<>
	void GetParentTypesOf<BuiltInTypeRegistrator>(const Callback<TypeId>& reportParentType) {
		reportParentType(TypeId::Of<Object>());
	}
}
