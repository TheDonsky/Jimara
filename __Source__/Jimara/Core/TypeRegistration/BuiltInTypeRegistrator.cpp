#include "TypeRegistartion.h"
#include "../../__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h"
#include "../../Core/Collections/ObjectCache.h"
#include "../../Core/Synch/SpinLock.h"
#include <shared_mutex>

namespace Jimara {
	bool TypeId::IsDerivedFrom(const TypeId& other)const {
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

		inline static EventInstance<>& TypeId_OnRegisteredSetChanged() {
			static EventInstance<> onChanged;
			return onChanged;
		}

		inline static SpinLock& CurrentRegisteredTypeSetLock() {
			static SpinLock lock;
			return lock;
		}

		inline static Reference<RegisteredTypeSet>& CurrentRegisteredTypeSet() {
			static Reference<RegisteredTypeSet> set;
			return set;
		}

		typedef void(*TypeId_RegistrationCallback)();

		class TypeId_RegistrationToken : public virtual ObjectCache<TypeId>::StoredObject {
		public:
			class Cache;

		private:
			const Reference<Cache> m_cache;
			const TypeId m_typeId;
			const TypeId_RegistrationCallback m_onUnregister;

		public:
			inline TypeId_RegistrationToken(const TypeId& typeId, const TypeId_RegistrationCallback& onRegister, const TypeId_RegistrationCallback& onUnregister) 
				: m_cache(Cache::Instance()), m_typeId(typeId), m_onUnregister(onUnregister) {
				const bool registered = [&]() {
					std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
					TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
					if (it == TypeId_GlobalRegistry().end()) {
						TypeId_GlobalRegistry()[m_typeId.TypeIndex()] = std::make_pair(m_typeId, 1);
						TypeId_TypeNameRegistry()[m_typeId.Name()] = m_typeId;
						onRegister();
						std::unique_lock<SpinLock> registeredTypeSetLock(CurrentRegisteredTypeSetLock());
						CurrentRegisteredTypeSet() = nullptr;
						return true;
					}
					else {
						it->second.second++;
						return false;
					}
				}();
				if (registered) 
					TypeId_OnRegisteredSetChanged()();
			}

			inline virtual ~TypeId_RegistrationToken() {
				const bool unregistered = [&]() {
					std::unique_lock<std::shared_mutex> lock(TypeId_RegistryLock());
					TypeId_Registry::iterator it = TypeId_GlobalRegistry().find(m_typeId.TypeIndex());
					if (it == TypeId_GlobalRegistry().end()) return false;
					if (it->second.second > 1) {
						it->second.second--;
						return false;
					}
					else {
						TypeId_GlobalRegistry().erase(it);
						TypeId_ByName::iterator ii = TypeId_TypeNameRegistry().find(m_typeId.Name());
						if (ii != TypeId_TypeNameRegistry().end() && ii->second == m_typeId)
							TypeId_TypeNameRegistry().erase(ii);
						m_onUnregister();
						std::unique_lock<SpinLock> registeredTypeSetLock(CurrentRegisteredTypeSetLock());
						CurrentRegisteredTypeSet() = nullptr;
						return true;
					}
				}();
				if (unregistered)
					TypeId_OnRegisteredSetChanged()();
			}

			class Cache : public virtual ObjectCache<TypeId> {
			public:
				inline static Reference<Cache> Instance() {
					static const Reference<Cache> instance = Object::Instantiate<Cache>();
					return instance;
				}

				inline static Reference<TypeId_RegistrationToken> GetToken(
					const TypeId& typeId, const TypeId_RegistrationCallback& onRegister, const TypeId_RegistrationCallback& onUnregister) {
					return Instance()->GetCachedOrCreate(typeId, [&]() -> Reference<TypeId_RegistrationToken> {
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
		const Reference<const RegisteredTypeSet> registeredTypes = RegisteredTypeSet::Current();
		for (size_t i = 0; i < registeredTypes->Size(); i++)
			reportType(registeredTypes->At(i));
	}

	Event<>& TypeId::OnRegisteredTypeSetChanged() {
		return TypeId_OnRegisteredSetChanged();
	}

	Reference<RegisteredTypeSet> RegisteredTypeSet::Current() {
		Reference<RegisteredTypeSet> set;
		{
			std::unique_lock<SpinLock> registeredTypeSetLock(CurrentRegisteredTypeSetLock());
			set = CurrentRegisteredTypeSet();
			if (set != nullptr) return set;
		}
		std::vector<TypeId> types;
		{
			std::shared_lock<std::shared_mutex> lock(TypeId_RegistryLock());
			for (TypeId_Registry::const_iterator it = TypeId_GlobalRegistry().begin(); it != TypeId_GlobalRegistry().end(); ++it)
				types.push_back(it->second.first);
		}
		{
			std::unique_lock<SpinLock> registeredTypeSetLock(CurrentRegisteredTypeSetLock());
			set = CurrentRegisteredTypeSet();
			if (set != nullptr) return set;
			else {
				set = CurrentRegisteredTypeSet() = new RegisteredTypeSet(std::move(types));
				set->ReleaseRef();
				return set;
			}
		}
	}

	template<>
	void TypeIdDetails::GetParentTypesOf<BuiltInTypeRegistrator>(const Callback<TypeId>& reportParentType) {
		reportParentType(TypeId::Of<Object>());
	}
}
