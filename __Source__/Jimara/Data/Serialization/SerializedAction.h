#pragma once
#include "Serializable.h"
#include "DefaultSerializer.h"
#include "../../Core/WeakReference.h"


namespace Jimara {
	namespace Serialization {
		template<typename ReturnType>
		class SerializedAction {
		public:
			class Instance : public virtual Object, public virtual Serializable {
			public:
				inline virtual ReturnType Invoke()const { return EmptyAction<ReturnType>(); }

				inline virtual size_t ArgumentCount()const { return 0u; }
			};

			class Provider {
			public:
				virtual void GetSerializedActions(Callback<SerializedAction> report) { Unused(report); }
			};

			template<typename... Args>
			struct Create {
				template<typename... ArgSerializers>
				inline static SerializedAction From(
					const std::string_view& name,
					const Function<ReturnType, Args...>& action,
					ArgSerializers... argSerializers);
			};

			inline const std::string& Name()const { return m_name; }

			inline Reference<Instance> CreateInstance()const { return m_createInstance(this); }

		private:
			std::string m_name;
			Callback<> m_baseAction = Callback<>(Unused<>);
			using SerializerList = Stacktor<Reference<const Object>, 4u>;
			SerializerList m_argumentSerializers;

			inline static Reference<Instance> CreateBaseInstance(const SerializedAction*) { return Object::Instantiate<Instance>(); }
			typedef Reference<Instance>(*CreateInstanceFn)(const SerializedAction*);
			CreateInstanceFn m_createInstance = SerializedAction::CreateBaseInstance;
			
			template<typename RV, typename... Args>
			inline static std::enable_if_t<std::is_same_v<RV, void>, void> EmptyAction(Args...) {};

			template<typename RV, typename... Args>
			inline static std::enable_if_t<!std::is_same_v<RV, void>, RV> EmptyAction(Args...) { return {}; };

			template<typename... Args>
			struct ArgList;

			template<>
			struct ArgList<> {
				inline void FillSerializers(size_t, const SerializerList&) {}
				template<typename... RestOfTheSerializerTypes>
				inline static void CollectSerializers(SerializerList&, RestOfTheSerializerTypes... restOfTheSerializers) {}
				template<typename... Args>
				struct Call {
					template<typename... PrevArgs>
					inline static ReturnType Make(const ArgList<>&, const Function<ReturnType, Args...>& action, const PrevArgs&... args) {
						return action(args...);
					}
				};
				inline void GetFields(Callback<SerializedObject> recordElement) {}
				static const constexpr size_t ARG_COUNT = 0u;
			};

			template<typename... Args>
			struct ArgList {
				template<typename First, typename... Rest>
				struct TypeHelper {
					using TypeA = First;
					using RestArgs = ArgList<Rest...>;
				};

				using TypeA = typename TypeHelper<Args...>::TypeA;
				using RestArgs = typename TypeHelper<Args...>::RestArgs;

				using TypeStorage_t = std::conditional_t<
					std::is_assignable_v<const Object*&, TypeA>,
					std::conditional_t<
					std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					WeakReference<std::remove_pointer_t<TypeA>>,
					Reference<std::remove_pointer_t<TypeA>>>,
					TypeA>;
				TypeStorage_t value = {};
				Reference<const Serialization::ItemSerializer::Of<TypeA>> serializer;
				RestArgs rest;

				inline void FillSerializers(size_t index, const SerializerList& serializers) {
					serializer = serializers[index];
					rest.FillSerializers(index + 1, serializers);
				}

				inline static void CollectSerializers(SerializerList& list) {
					list.Push(DefaultSerializer<TypeA>::Create(""));
					RestArgs::CollectSerializers(list);
				}

				template<typename Serializer_t, typename... RestOfTheSerializerTypes>
				inline static
					std::enable_if_t<std::is_assignable_v<const Serialization::ItemSerializer*&, Serializer_t>, void>
					CollectSerializers(SerializerList& list, Serializer_t serializer, RestOfTheSerializerTypes... restOfTheSerializers) {
					const Serialization::ItemSerializer* ser = serializer;
					list.Push(serializer);
					RestArgs::CollectSerializers(list, restOfTheSerializers...);
				}

				template<typename... RestOfTheSerializerTypes>
				inline static void CollectSerializers(SerializerList& list,
					const std::string_view& name,
					RestOfTheSerializerTypes... restOfTheSerializers) {
					list.Push(DefaultSerializer<TypeA>::Create(name));
					RestArgs::CollectSerializers(list, restOfTheSerializers...);
				}

				template<typename... Args>
				struct Call {
					template<typename... PrevArgs>
					inline static void Make(const ArgList& args, const Function<ReturnType, Args...>& action, const PrevArgs&... prevArgs) {
						return RestArgs::
							template Call<Args...>::
							template Make<PrevArgs..., const TypeStorage_t&>(args.rest, action, prevArgs..., args.value);
					}
				};

				inline void GetFields(Callback<SerializedObject> recordElement) {
					if (serializer != nullptr) {
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
					}
					rest.GetFields(recordElement);
				}

				static const constexpr size_t ARG_COUNT = RestArgs::ARG_COUNT + 1;
			};

			template<typename... Args>
			struct ConcreteInstance : public virtual Instance {
				Function<ReturnType, Args...> action = Function<ReturnType, Args...>(
					SerializedAction<ReturnType>::template EmptyAction<ReturnType, Args...>);
				SerializedAction<ReturnType>::ArgList<Args...> arguments;

				inline virtual ReturnType Invoke()const override { return SerializedAction::ArgList<Args...>::template Call<Args...>::template Make<>(arguments, action); }
				inline virtual size_t ArgumentCount()const { return SerializedAction::ArgList<Args...>::ARG_COUNT; }
				inline virtual void GetFields(Callback<SerializedObject> recordElement) override { arguments.GetFields(recordElement); }
			};
		};

		using SerializedCallback = SerializedAction<void>;



		static_assert(std::is_same_v<void, void>);
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
		static_assert(std::is_assignable_v<const Object*&, SerializedAction<void>::Instance*>);
		static_assert(std::is_assignable_v<const Object*&, const SerializedAction<void>::Instance*>);
		static_assert(!std::is_same_v<int, int&>);
		static_assert(std::is_same_v<int&, std::conditional_t<std::is_same_v<int, int>, int&, int>>);

		template<typename ReturnType>
		template<typename... Args>
		template<typename... ArgSerializers>
		inline SerializedAction<ReturnType> SerializedAction<ReturnType>::Create<Args...>::From(
			const std::string_view& name,
			const Function<ReturnType, Args...>& action,
			ArgSerializers... argSerializers) {
			static_assert(sizeof(SerializedAction::m_baseAction) == sizeof(const Function<ReturnType, Args...>));

			SerializedAction result = {};
			result.m_name = name;
			result.m_baseAction = *reinterpret_cast<const Callback<>*>(reinterpret_cast<const void*>(&action));
			SerializedAction<ReturnType>::ArgList<Args...>::CollectSerializers(result.m_argumentSerializers, argSerializers...);

			result.m_createInstance = [](const SerializedAction* act) -> Reference<Instance> {
				Reference<ConcreteInstance<Args...>> instance = Object::Instantiate<ConcreteInstance<Args...>>();
				instance->action = *reinterpret_cast<const Function<ReturnType, Args...>*>(reinterpret_cast<const void*>(&act->m_baseAction));
				instance->arguments.FillSerializers(0u, act->m_argumentSerializers);
				return instance;
			};

			return result;
		}
	}
}
