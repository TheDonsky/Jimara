#include "TypeRegistartion.h"
#include "../../__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h"
#include "../../Core/Collections/ObjectCache.h"
#include <shared_mutex>

namespace Jimara {
	inline bool TypeId::IsDerivedFrom(const TypeId& other)const {
		if (other == (*this)) return true;
		bool result = false;
		IterateParentTypes([&](const TypeId& parentType) {
			if (result) return;
			else if (parentType == other) result = true;
			else result = parentType.IsDerivedFrom(other);
			});
		return result;
	}

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

		typedef std::unordered_map<std::string_view, TypeId> TypeId_ByName;
		inline static TypeId_ByName& TypeId_TypeNameRegistry() {
			static TypeId_ByName registry;
			return registry;
		}

		typedef void(*TypeId_RegistrationCallback)();

		class TypeId_RegistrationToken : public virtual ObjectCache<TypeId>::StoredObject {
		private:
			const TypeId m_typeId;
			const TypeId_RegistrationCallback m_onUnregister;

		public:
			inline TypeId_RegistrationToken(const TypeId& typeId, const TypeId_RegistrationCallback& onRegister, const TypeId_RegistrationCallback& onUnregister) 
				: m_typeId(typeId), m_onUnregister(onUnregister) {
				std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
				TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
				if (it == TypeId_GlobalRegistry().end()) {
					TypeId_GlobalRegistry()[m_typeId.TypeIndex()] = std::make_pair(m_typeId, 1);
					TypeId_TypeNameRegistry()[m_typeId.Name()] = m_typeId;
					onRegister();
				}
				else it->second.second++;
			}

			inline virtual ~TypeId_RegistrationToken() {
				std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
				TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
				if (it == TypeId_GlobalRegistry().end()) return;
				if (it->second.second > 1)
					it->second.second--;
				else {
					TypeId_GlobalRegistry().erase(it);
					TypeId_ByName::iterator ii = TypeId_TypeNameRegistry().find(m_typeId.Name());
					if (ii != TypeId_TypeNameRegistry().end() && ii->second == m_typeId)
						TypeId_TypeNameRegistry().erase(ii);
					m_onUnregister();
				}
			}

			class Cache : public virtual ObjectCache<TypeId> {
			public:
				inline static Reference<TypeId_RegistrationToken> GetToken(
					const TypeId& typeId, const TypeId_RegistrationCallback& onRegister, const TypeId_RegistrationCallback& onUnregister) {
					static Cache cache;
					return cache.GetCachedOrCreate(typeId, false, [&]() -> Reference<TypeId_RegistrationToken> {
						return Object::Instantiate<TypeId_RegistrationToken>(typeId, onRegister, onUnregister);
						});
				}
			};
		};
	}

	Reference<Object> TypeId::Register()const {
		RegistrationCallback onRegister, onUnregister;
		m_registrationCallbackGetter(onRegister, onUnregister);
		return TypeId_RegistrationToken::Cache::GetToken(*this, onRegister, onUnregister);
	}

	bool TypeId::Find(const std::type_info& typeInfo, TypeId& result) {
		std::shared_lock<std::shared_mutex> lock(TypeId_RegistryLock());
		TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(typeInfo);
		if (it == TypeId_GlobalRegistry().end()) return false;
		else {
			result = it->second.first;
			return true;
		}
	}

	bool TypeId::Find(const std::string_view& typeName, TypeId& result) {
		std::shared_lock<std::shared_mutex> lock(TypeId_RegistryLock());
		TypeId_ByName::iterator it = TypeId_TypeNameRegistry().find(typeName);
		if (it == TypeId_TypeNameRegistry().end()) return false;
		else {
			result = it->second;
			return true;
		}
	}

	void TypeId::GetRegisteredTypes(const Callback<TypeId>& reportType) {
		std::shared_lock<std::shared_mutex> lock(TypeId_RegistryLock());
		for (TypeId_Registry::const_iterator it = TypeId_GlobalRegistry().begin(); it != TypeId_GlobalRegistry().end(); ++it)
			reportType(it->second.first);
	}

	template<>
	void TypeIdDetails::GetParentTypesOf<BuiltInTypeRegistrator>(const Callback<TypeId>& reportParentType) {
		reportParentType(TypeId::Of<Object>());
	}
}
