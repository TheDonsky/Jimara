#pragma once
#include "Serializable.h"
#include "DefaultSerializer.h"
#include "../../Core/WeakReference.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Arbitrary action, that can be represented with a function and expected argument list;
		/// <para/> To invoke it, one needs to create it's instance, assign argument values through standard serialization utilities 
		/// and make the call through SerializedAction&lt;Type&gt;::Instance::Invoke() method.
		/// </summary>
		/// <typeparam name="ReturnType"> Action return value type </typeparam>
		template<typename ReturnType>
		class SerializedAction {
		public:
			/// <summary>
			/// Action-instance, holding the underlying function alonside the serializable argument list;
			/// <para/> Arguments can be manipulated through Serializable::GetFields interface;
			/// <para/> To invoke the function with the instance arguments, you can use Instance::Invoke() method;
			/// <para/> Object-pointer arguments in general will be stored as References, but if they are Weakly-referenceable, 
			/// WeakReference will be used instead within internal storage.
			/// </summary>
			class Instance : public virtual Object, public virtual Serializable {
			public:
				/// <summary>
				/// Invokes the underlying function with serialized arguments
				/// </summary>
				/// <returns> Function's return value </returns>
				inline virtual ReturnType Invoke()const { return EmptyAction<ReturnType>(); }

				/// <summary> Number of arguments, the method expects </summary>
				inline virtual size_t ArgumentCount()const { return 0u; }
			};

			/// <summary> Inteface for an object that can report any number of SerializedAction records </summary>
			class Provider {
			public:
				/// <summary>
				/// Reports actions associated with this object.
				/// </summary>
				/// <param name="report"> Actions will be reported through this callback </param>
				virtual void GetSerializedActions(Callback<SerializedAction> report) { Unused(report); }

				/// <summary>
				/// Reports actions associated with this object.
				/// </summary>
				/// <typeparam name="CallType"> Arbitrary callable that can receive SerializedAction as an argument </typeparam>
				/// <param name="report"> Actions will be reported through this callback [Like report(SerializedAction())] </param>
				template<typename CallType>
				inline std::enable_if_t<!std::is_same_v<Callback<SerializedAction>, CallType>, void> GetSerializedActions(const CallType& report) {
					GetSerializedActions(Callback<SerializedAction>::FromCall(&report));
				}
			};

			/// <summary>
			/// Interface for creating SerializedAction records without complications
			/// <para/> For example, one could create SerializedAction using
			/// SerializedAction&lt;void&gt;::Create&lt;args...&gt;::From('Function name/id', Callback&lt;args...$gt;, names/serializers);
			/// </summary>
			/// <typeparam name="...Args"> Function argument list </typeparam>
			template<typename... Args>
			class Create;

			/// <summary> Name of the serialized-action (keep the names small and they might just be subject to small-string optimization) </summary>
			inline const std::string& Name()const { return m_name; }

			/// <summary>
			/// Creates an instance with the serialized argument-block
			/// </summary>
			/// <returns> New instance of the serialized action </returns>
			inline Reference<Instance> CreateInstance()const { return m_createInstance(this); }


		private:
			// Name of the serialized-action (keep the names small and they might just be subject to small-string optimization)
			std::string m_name;

			// Underlying action
			Callback<> m_baseAction = Callback<>(Unused<>);

			// Serializer list
			using SerializerList = Stacktor<Reference<const Object>, 4u>;
			SerializerList m_argumentSerializers;

			// Instance-create function
			inline static Reference<Instance> CreateBaseInstance(const SerializedAction*) { return Object::Instantiate<Instance>(); }
			typedef Reference<Instance>(*CreateInstanceFn)(const SerializedAction*);
			CreateInstanceFn m_createInstance = SerializedAction::CreateBaseInstance;
			
			// Empty/Default function
			template<typename RV, typename... Args>
			inline static std::enable_if_t<std::is_same_v<RV, void>, void> EmptyAction(Args...) {};

			// Empty/Default function
			template<typename RV, typename... Args>
			inline static std::enable_if_t<!std::is_same_v<RV, void>, RV> EmptyAction(Args...) { return {}; };

			// Underlying implementation for concrete types resides in-here.
			struct Helpers;
		};


		/// <summary> SerializedAction with no return value </summary>
		using SerializedCallback = SerializedAction<void>;


		/// <summary>
		/// Interface for creating SerializedAction records without complications
		/// <para/> For example, one could create SerializedAction using
		/// SerializedAction&lt;void&gt;::Create&lt;args...&gt;::From('Function name/id', Callback&lt;args...$gt;, names/serializers);
		/// </summary>
		/// <typeparam name="ReturnType"> Function return value </typeparam>
		/// <typeparam name="...Args"> Function argument list </typeparam>
		template<typename ReturnType>
		template<typename... Args>
		class SerializedAction<ReturnType>::Create {
		public:
			/// <summary>
			/// Creates SerializedAction of given name with given underlying function and the argument names or serializers;
			/// <para/> If argSerializers has more entries than the number of arguments the function expects, the tail-end of the sequence will be ignored;
			/// <para/> If argSerializers has less entries than the number of arguments the function expects, the remainder of the arguments will be assigned unnamed serializers;
			/// <para/> If argSerializers contains nulls, the effected arguments will not be serialized.
			/// </summary>
			/// <typeparam name="...ArgSerializers"> A list of argument serializer references or just argument names for internal serializer-creation </typeparam>
			/// <param name="name"> Function name </param>
			/// <param name="action"> Underlying action to use </param>
			/// <param name="...argSerializers"> Serializers and/or argument names; in the order of the arguments </param>
			/// <returns> SerializedAction </returns>
			template<typename... ArgSerializers>
			inline static SerializedAction From(
				const std::string_view& name,
				const Function<ReturnType, Args...>& action,
				ArgSerializers... argSerializers);
		};




		// Underlying implementation-helpers for the SerializedAction; NOT RELEVANT for the public API.
		template<typename ReturnType>
		struct SerializedAction<ReturnType>::Helpers {
			// Argument list
			template<typename... Args>
			struct ArgList;

			// Override for the argument-list for no-arg case
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

			// 'Normal' argument list with nested structure
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
					inline static ReturnType Make(const ArgList& args, const Function<ReturnType, Args...>& action, const PrevArgs&... prevArgs) {
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

			// Instance of a concrete underlying types
			template<typename... Args>
			struct ConcreteInstance : public virtual Instance {
				Function<ReturnType, Args...> action = Function<ReturnType, Args...>(
					SerializedAction<ReturnType>::template EmptyAction<ReturnType, Args...>);
				SerializedAction<ReturnType>::Helpers::ArgList<Args...> arguments;

				inline virtual ReturnType Invoke()const override { return ArgList<Args...>::template Call<Args...>::template Make<>(arguments, action); }
				inline virtual size_t ArgumentCount()const { return ArgList<Args...>::ARG_COUNT; }
				inline virtual void GetFields(Callback<SerializedObject> recordElement) override { arguments.GetFields(recordElement); }
			};
		};


		// A few sanity-checks that the internal implementation assumes are true..
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

		/// <summary>
		/// Creates SerializedAction of given name with given underlying function and the argument names or serializers;
		/// <para/> If argSerializers has more entries than the number of arguments the function expects, the tail-end of the sequence will be ignored;
		/// <para/> If argSerializers has less entries than the number of arguments the function expects, the remainder of the arguments will be assigned unnamed serializers;
		/// <para/> If argSerializers contains nulls, the effected arguments will not be serialized.
		/// </summary>
		/// <typeparam name="...ArgSerializers"> A list of argument serializer references or just argument names for internal serializer-creation </typeparam>
		/// <param name="name"> Function name </param>
		/// <param name="action"> Underlying action to use </param>
		/// <param name="...argSerializers"> Serializers and/or argument names; in the order of the arguments </param>
		/// <returns> SerializedAction </returns>
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
			SerializedAction<ReturnType>::Helpers::ArgList<Args...>::CollectSerializers(result.m_argumentSerializers, argSerializers...);

			result.m_createInstance = [](const SerializedAction* act) -> Reference<Instance> {
				Reference<Helpers::ConcreteInstance<Args...>> instance = Object::Instantiate<Helpers::ConcreteInstance<Args...>>();
				instance->action = *reinterpret_cast<const Function<ReturnType, Args...>*>(reinterpret_cast<const void*>(&act->m_baseAction));
				instance->arguments.FillSerializers(0u, act->m_argumentSerializers);
				return instance;
			};

			return result;
		}
	}
}
