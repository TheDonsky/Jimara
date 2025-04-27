#pragma once
#include "Serializable.h"
#include "DefaultSerializer.h"
#include "../../Core/WeakReference.h"


namespace Jimara {
	namespace Serialization {
		class JIMARA_API SerializedAction {
		public:
			class JIMARA_API Instance;

			struct JIMARA_API ArgInfo;

			template<typename... Args, typename... ArgSerializers>
			inline static SerializedAction Create(
				const std::string_view& name, 
				const Callback<Args...>& callback,
				ArgSerializers... argSerializers);

			inline const std::string& Name()const { return m_name; }

			inline Reference<Instance> CreateInstance()const;

		private:
			std::string m_name;
			Callback<> m_baseAction;
			using SerializerList = Stacktor<Reference<const Object>, 4u>;
			SerializerList m_argumentSerializers;

			inline static Reference<Instance> CreateBaseInstance(const SerializedAction*) { return Object::Instantiate<Instance>(); }
			typedef Reference<Instance>(*CreateInstanceFn)(const SerializedAction*);
			CreateInstanceFn m_createInstance = SerializedAction::CreateBaseInstance;

			template<typename... Args>
			struct ArgList;

			template<typename... Args>
			struct ConcreteInstance;
		};


		class JIMARA_API SerializedAction::Instance : public virtual Object, public virtual Serializable {
		public:
			inline virtual void Invoke()const {}
		};

		struct JIMARA_API SerializedAction::ArgInfo {
			std::string_view name;
			std::vector<Reference<const Object>> serializationAttributes;
		};

		inline Reference<SerializedAction::Instance> SerializedAction::CreateInstance()const { return m_createInstance(this); }

		template<>
		struct SerializedAction::ArgList<> {
			inline void FillSerializers(size_t, const SerializerList&) {}
			inline static void CollectSerializers(SerializerList&) {}
			template<typename... Args>
			inline void Call(const Callback<Args...>& callback, const Args&... arguments) { callback(arguments...); }
			inline void GetFields(Callback<SerializedObject> recordElement) {}
		};

		static_assert(!std::is_assignable_v<const Object*&, Object>);
		static_assert(std::is_assignable_v<const Object*&, Object*>);
		static_assert(!std::is_assignable_v<const Object*&, const Object>);
		static_assert(std::is_assignable_v<const Object*&, const Object*>);
		static_assert(!std::is_assignable_v<const Object*&, Serializable>);
		static_assert(!std::is_assignable_v<const Object*&, Serializable*>);
		static_assert(!std::is_assignable_v<const Object*&, const Serializable>);
		static_assert(!std::is_assignable_v<const Object*&, const Serializable*>);
		static_assert(!std::is_assignable_v<const Object*&, WeaklyReferenceable>);
		static_assert(std::is_assignable_v<const Object*&, WeaklyReferenceable*>);
		static_assert(std::is_assignable_v<const Object*&, const WeaklyReferenceable*>);
		static_assert(std::is_assignable_v<const Object*&, SerializedAction::Instance*>);
		static_assert(std::is_assignable_v<const Object*&, const SerializedAction::Instance*>);

		static_assert(!std::is_same_v<int, int&>);
		static_assert(std::is_same_v<int&, std::conditional_t<std::is_same_v<int, int>, int&, int>>);

		template<typename TypeA, typename... Rest>
		struct SerializedAction::ArgList<TypeA, Rest...> {
			using TypeStorage_t = std::conditional_t<
				std::is_assignable_v<const Object*&, TypeA>,
				std::conditional_t<
					std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					WeakReference<std::remove_pointer_t<TypeA>>,
					Reference<std::remove_pointer_t<TypeA>>>,
				TypeA>;
			TypeStorage_t value;
			Reference<const Serialization::ItemSerializer::Of<TypeA>> serializer;
			SerializedAction::ArgList<Rest...> rest;

			inline void FillSerializers(size_t index, const SerializerList& serializers) {
				serializer = serializers[index];
				rest.FillSerializers(index + 1, serializers);
			}

			template<typename... RestOfTheSerializerTypes>
			inline static void CollectSerializers(SerializerList& list, 
				const Serialization::ItemSerializer::Of<TypeA>* serializer, 
				RestOfTheSerializerTypes... restOfTheSerializers) {
				list.Push(serializer);
				SerializedAction::ArgList<Rest...>::CollectSerializers(list, restOfTheSerializers...);
			}

			template<typename... Args, typename... PrevArgs>
			inline void Call(const Callback<Args...>& callback, const PrevArgs&... prevArgs) {
				rest.Call(callback, prevArgs, value);
			}

			inline void GetFields(Callback<SerializedObject> recordElement) {
				using TypeRef_t = TypeA&;
				static_assert(!std::is_same_v<TypeRef_t, TypeA>);
				using WrappedType_t = std::conditional_t<
					std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					Reference<std::remove_pointer_t<TypeA>>,
					TypeRef_t>;
				WrappedType_t wrapped = value;
				recordElement(serializer->Serialize(value));
				if (!std::is_same_v<WrappedType_t, TypeRef_t>)
					value = wrapped;
				rest.GetFields(recordElement);
			}
		};

		template<typename... Args>
		struct SerializedAction::ConcreteInstance : public virtual Instance {
			Callback<Args...> action;
			SerializedAction::ArgList<Args...> arguments;

			inline virtual void Invoke()const override { arguments.Call(action); }
			inline virtual void GetFields(Callback<SerializedObject> recordElement) override { arguments.GetFields(recordElement); }
		};

		template<typename... Args, typename... ArgSerializers>
		inline SerializedAction SerializedAction::Create(
			const std::string_view& name,
			const Callback<Args...>& callback,
			ArgSerializers... argSerializers) {
			static_assert(sizeof(SerializedAction::m_baseAction) == sizeof(const Callback<Args...>));

			SerializedAction action = {};
			action.m_name = name;
			action.m_baseAction = *reinterpret_cast<const Callback<>*>(reinterpret_cast<const void*>(&callback));
			ArgList<Args...>::CollectSerializers(action.m_argumentSerializers, argSerializers...);

			action.m_createInstance = [](SerializedAction* act) {
				Reference<ConcreteInstance> instance = Object::Instantiate<ConcreteInstance>();
				instance->action = act->m_baseAction;
				instance->arguments.FillSerializers(0u, act->m_argumentSerializers);
				return instance;
			};

			return action;
		}
	}
}
