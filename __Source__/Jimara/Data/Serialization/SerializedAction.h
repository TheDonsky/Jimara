#pragma once
#include "Serializable.h"
#include "DefaultSerializer.h"
#include "../../Core/WeakReference.h"
#include "Attributes/DefaultValueAttribute.h"


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
			/// WeakReference will be used instead within internal storage 
			/// [will still get serialized as Reference, though and in that case, keeping the serialized object beyond the relevant scope will be unsafe].
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
			class Provider : public virtual Object {
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
			/// Basic field information
			/// <para/> Can be passed as an argument to create-functions, instead of the name/serializer
			/// </summary>
			/// <typeparam name="FieldType"></typeparam>
			template<typename FieldType>
			struct FieldInfo {
				/// <summary> Feild name (will be interpreted as the serializer-name) </summary>
				std::string_view fieldName = "";

				/// <summary> Feild hint/description (will be interpreted as the serializer-hint) </summary>
				std::string_view fieldHint = "";

				/// <summary> Feild default value (will create a default-value attribute) </summary>
				FieldType defaultValue = {};
			};

			/// <summary>
			/// Interface for creating SerializedAction records without complications
			/// <para/> For example, one could create SerializedAction using
			/// SerializedAction&lt;void&gt;::Create&lt;args...&gt;::From('Function name/id', Callback&lt;args...$gt;, names/serializers);
			/// </summary>
			/// <typeparam name="...Args"> Function argument list </typeparam>
			template<typename... Args>
			class Create;

			/// <summary> 
			/// Serializable instance alongside the corresponding action-provider
			/// <para/> This class is not designed to be thread-safe; 
			/// Provider changes, serialization and invokation can not safetly overlap.
			/// </summary>
			class ProvidedInstance;

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
			/// <param name="...argSerializers"> Serializers and/or argument names and/or FieldInfo entries; in the order of the arguments </param>
			/// <returns> SerializedAction </returns>
			template<typename... ArgSerializers>
			inline static SerializedAction From(
				const std::string_view& name,
				const Function<ReturnType, Args...>& action,
				ArgSerializers... argSerializers);
		};


		/// <summary> 
		/// Serializable instance alongside the corresponding action-provider
		/// <para/> This class is not designed to be thread-safe; 
		/// Provider changes, serialization and invokation can not safetly overlap.
		/// </summary>
		template<typename ReturnType>
		class SerializedAction<ReturnType>::ProvidedInstance : public virtual Instance {
		public:
			/// <summary> Action source </summary>
			inline Reference<Provider> ActionProvider()const { return Helpers::ActionProvider(this); }

			/// <summary>
			/// Sets action source
			/// </summary>
			/// <param name="provider"> Action-Provider to use </param>
			/// <param name="clearAction"> 
			/// If true, the underlying stored action will be cleared and no attempt will be made to find an equivalent from the new provider 
			/// <para/> Underlying action will not be cleared, if the action-provider does not change after the call.
			/// </param>
			/// <param name="keepArgumentValues"> 
			/// If true and if 'clearAction' is set to false, the argument values will be copied from the old action if the adequate equivalent action is found 
			/// </param>
			inline void SetActionProvider(Provider* provider, bool clearAction = false, bool keepArgumentValues = true) { 
				Helpers::SetActionProvider(this, provider, clearAction, keepArgumentValues); 
			}

			/// <summary> Name of the currently set action (may remain unchanged even if the provider is destroyed) </summary>
			inline const std::string& ActionName()const { return m_action.Name(); }

			/// <summary>
			/// Tries to find and set the action based on the name
			/// </summary>
			/// <param name="actionName"> Action name to search for </param>
			/// <param name="keepArgumentValues"> 
			/// If true, the argument values will be copied from the old action if the new action has the same signature
			/// </param>
			inline void SetActionByName(const std::string_view& actionName, bool keepArgumentValues = true) {  
				Helpers::SetProviderActionByName(this, actionName, keepArgumentValues);
			}

			/// <summary>
			/// Invokes the underlying function with serialized arguments
			/// <para/> If the weakly-referenceable provider is lost, the action will not be invoked for safety reasons.
			/// </summary>
			/// <returns> Function's return value </returns>
			inline virtual ReturnType Invoke()const override { 
				Reference<Provider> provider = ActionProvider();
				Reference<Instance> instance = m_actionInstance;
				if (provider != nullptr && instance != nullptr)
					return instance->Invoke(); 
			}

			/// <summary> Number of arguments the method expects </summary>
			inline virtual size_t ArgumentCount()const override { 
				Reference<Instance> instance = m_actionInstance;
				return (instance == nullptr) ? 0u : instance->ArgumentCount();
			}

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
			virtual void GetFields(Callback<SerializedObject> recordElement)override { Helpers::GetProvidedInstanceFields(this, recordElement); }

		private:
			// If the provider happens to be weakly-referenceable, we will only store thre weak-reference
			WeakReference<WeaklyReferenceable> m_providerWeak;

			// If the provider is not weakly-referenceable, we will store it here
			Reference<Provider> m_providerStrong;

			// Underlying action
			SerializedAction m_action;

			// Underlying action instance
			Reference<Instance> m_actionInstance;

			// Helpers contain most of the actual implementation
			friend struct Helpers;
		};




		// Underlying implementation-helpers for the SerializedAction; NOT RELEVANT for the public API.
		template<typename ReturnType>
		struct SerializedAction<ReturnType>::Helpers {
			// Argument-list for no-arg case
			struct SingleArgList {
				inline ~SingleArgList() {}
				inline void FillSerializers(size_t, const SerializerList&) {}
				template<typename... RestOfTheSerializerTypes>
				inline static void CollectSerializers(SerializerList&, RestOfTheSerializerTypes... restOfTheSerializers) {}
				template<typename... Args>
				struct Call {
					template<typename... PrevArgs>
					inline static ReturnType Make(const SingleArgList&, const Function<ReturnType, Args...>& action, const PrevArgs&... args) {
						return action(args...);
					}
				};
				inline void GetFields(Callback<SerializedObject> recordElement) {}
				inline bool SerializerListValid(size_t, const SerializerList&)const { return true; }
				inline void CopyArguments(SingleArgList&)const {}
				static const constexpr size_t ARG_COUNT = 0u;
			};

			// 'Normal' argument list with nested structure
			template<typename... Args>
			struct MultiArgList;

			// General argument-list
			template<typename... Args>
			using ArgList = typename std::conditional<(sizeof...(Args)) <= 0, SingleArgList, MultiArgList<Args...>>::type;

			// 'Normal' argument list with nested structure
			template<typename... Arguments>
			struct MultiArgList {
				template<typename First, typename... Rest>
				struct TypeHelper {
					using TypeA = std::remove_const_t<std::remove_reference_t<First>>;
					using RestArgs = ArgList<Rest...>;
				};

				using CallType = typename TypeHelper<Arguments...>::TypeA;
				using TypeA = typename std::conditional_t<
					std::is_enum_v<CallType>,
					std::underlying_type<CallType>,
					std::enable_if<true, CallType>>::type;
				using RestArgs = typename TypeHelper<Arguments...>::RestArgs;

				using TypeStorage_t = std::conditional_t<
					std::is_pointer_v<TypeA>&& std::is_assignable_v<const Object*&, TypeA>,
					std::conditional_t<
					std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					WeakReference<std::remove_pointer_t<TypeA>>,
					Reference<std::remove_pointer_t<TypeA>>>,
					TypeA>;

				using StoredTypeRef_t = TypeStorage_t&;

				static_assert(!std::is_same_v<StoredTypeRef_t, TypeA>);

				using WrappedType_t = std::conditional_t<
					std::is_pointer_v<TypeA>&& std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					Reference<std::remove_pointer_t<TypeA>>,
					StoredTypeRef_t>;

				using ConstWrappedType_t = std::conditional_t<
					std::is_pointer_v<TypeA>&& std::is_assignable_v<const WeaklyReferenceable*&, TypeA>,
					Reference<std::remove_pointer_t<TypeA>>,
					const TypeStorage_t&>;

				using SerializerTarget_t = std::remove_reference_t<WrappedType_t>;

				TypeStorage_t value = {};
				using ReferencedSerializer_t = const Serialization::ItemSerializer::Of<SerializerTarget_t>;
				Reference<ReferencedSerializer_t> serializer;
				RestArgs rest;

				inline ~MultiArgList() {}

				inline void FillSerializers(size_t index, const SerializerList& serializers) {
					serializer = serializers[index];
					if (serializer != nullptr) {
						const DefaultValueAttribute<TypeA>* defaultValue = serializer->template FindAttributeOfType<DefaultValueAttribute<TypeA>>();
						if (defaultValue != nullptr)
							value = defaultValue->value;
					}
					rest.FillSerializers(index + 1, serializers);
				}

				inline static void CollectSerializers(SerializerList& list) {
					list.Push(DefaultSerializer<SerializerTarget_t>::Create(""));
					RestArgs::CollectSerializers(list);
				}

				template<typename Serializer_t>
				static const constexpr bool IsSerializerReference = std::is_assignable_v<const Serialization::ItemSerializer*&, Serializer_t>;

				template<typename String_t>
				static const constexpr bool IsStringType = std::is_assignable_v<std::string_view, String_t>;

				template<typename Serializer_t, typename... RestOfTheSerializerTypes>
				inline static std::enable_if_t<IsSerializerReference<Serializer_t>, void>
					CollectSerializers(SerializerList& list, Serializer_t serializer, RestOfTheSerializerTypes... restOfTheSerializers) {
					const Serialization::ItemSerializer* ser = serializer;
					list.Push(serializer);
					RestArgs::CollectSerializers(list, restOfTheSerializers...);
				}

				template<typename String_t, typename... RestOfTheSerializerTypes>
				inline static std::enable_if_t<IsStringType<String_t>, void>
					CollectSerializers(SerializerList& list, const String_t& name, RestOfTheSerializerTypes... restOfTheSerializers) {
					list.Push(DefaultSerializer<SerializerTarget_t>::Create((std::string_view)name));
					RestArgs::CollectSerializers(list, restOfTheSerializers...);
				}

				template<typename FieldInfo_t, typename... RestOfTheSerializerTypes>
				inline static std::enable_if_t<(!IsSerializerReference<FieldInfo_t>) && (!IsStringType<FieldInfo_t>), void>
					CollectSerializers(SerializerList& list, const FieldInfo_t& fieldInfo, RestOfTheSerializerTypes... restOfTheSerializers) {
					const FieldInfo<TypeA> info = fieldInfo;
					list.Push(DefaultSerializer<SerializerTarget_t>::Create(info.fieldName, info.fieldHint,
						{ Object::Instantiate<DefaultValueAttribute<TypeA>>(info.defaultValue) }));
					RestArgs::CollectSerializers(list, restOfTheSerializers...);
				}

				template<typename... Args>
				struct Call {
					template<typename... PrevArgs>
					inline static ReturnType Make(const MultiArgList& args, const Function<ReturnType, Args...>& action, const PrevArgs&... prevArgs) {
						ConstWrappedType_t valRef = args.value;
						return RestArgs::
							template Call<Args...>::
							template Make<PrevArgs..., const CallType&>(args.rest, action, prevArgs...,
								*reinterpret_cast<const CallType*>(reinterpret_cast<const void*>(&valRef)));
					}
				};

				inline void GetFields(Callback<SerializedObject> recordElement) {
					if (serializer != nullptr) {
						WrappedType_t wrapped = value;
						recordElement(serializer->Serialize(wrapped));
						if (!std::is_same_v<WrappedType_t, StoredTypeRef_t>)
							value = wrapped;
					}
					rest.GetFields(recordElement);
				}

				inline bool SerializerListValid(size_t curIndex, const SerializerList& list)const {
					if (serializer != nullptr) {
						if (curIndex >= list.Size())
							return false;
						Reference<ReferencedSerializer_t> candidate = list[curIndex];
						if (candidate == nullptr)
							return false;
						if (dynamic_cast<const ItemSerializer*>(candidate.operator->())->TargetName() !=
							dynamic_cast<const ItemSerializer*>(serializer.operator->())->TargetName())
							return false;
					}
					return rest.SerializerListValid(curIndex + 1u, list);
				}

				inline void CopyArguments(MultiArgList& dst)const {
					if (serializer != nullptr && dst.serializer != nullptr)
						dst.value = value;
					rest.CopyArguments(dst.rest);
				}

				static const constexpr size_t ARG_COUNT = RestArgs::ARG_COUNT + 1;
			};

			// Instance of a concrete underlying types
			struct BaseConcreteInstance : public virtual Object {
				inline virtual bool SerializerListValid(const SerializerList& list)const = 0;
				inline virtual void CopyArgumentValues(Instance* dst)const = 0;
			};
			template<typename... Args>
			struct ConcreteInstance 
				: public virtual Instance
				, public virtual BaseConcreteInstance {
				Function<ReturnType, Args...> action = Function<ReturnType, Args...>(
					SerializedAction<ReturnType>::template EmptyAction<ReturnType, Args...>);
				SerializedAction<ReturnType>::Helpers::ArgList<Args...> arguments;

				inline virtual ~ConcreteInstance() {}

				inline virtual ReturnType Invoke()const override { return decltype(arguments)::template Call<Args...>::template Make<>(arguments, action); }
				inline virtual size_t ArgumentCount()const { return decltype(arguments)::ARG_COUNT; }
				inline virtual void GetFields(Callback<SerializedObject> recordElement) override { arguments.GetFields(recordElement); }
				inline virtual bool SerializerListValid(const SerializerList& list)const override { return arguments.SerializerListValid(0u, list); }
				inline virtual void CopyArgumentValues(Instance* dst)const override {
					ConcreteInstance<Args...>* destination = dynamic_cast<ConcreteInstance<Args...>*>(dst);
					if (destination != nullptr)
						arguments.CopyArguments(destination->arguments);
				}
			};


			// Implementation of ProvidedInstance:
			inline static Reference<Provider> ActionProvider(const ProvidedInstance* self) {
				Reference<WeaklyReferenceable> weakRv = self->m_providerWeak;
				Reference<Provider> rv = weakRv;
				if (rv == nullptr)
					rv = self->m_providerStrong;
				return rv;
			}

			inline static void SetActionProvider(ProvidedInstance* self, Provider* provider, bool clearAction, bool keepArgumentValues) {
				const Reference<Provider> oldProvider = ActionProvider(self);
				if (oldProvider == provider)
					return;
				// Clear:
				{
					self->m_providerWeak = nullptr;
					self->m_providerStrong = nullptr;
				}
				// Fill:
				{
					WeaklyReferenceable* weakProvider = dynamic_cast<WeaklyReferenceable*>(provider);
					if (weakProvider != nullptr)
						self->m_providerWeak = weakProvider;
					else self->m_providerStrong = provider;
				}
				const Reference<Provider> curProvider = self->ActionProvider();
				assert(curProvider == provider);
				// Clear action if there's no provider to speak of, or if we don't want to keep an existing action:
				if (oldProvider == nullptr || curProvider == nullptr || clearAction || self->m_actionInstance == nullptr) {
					self->m_action = SerializedAction();
					self->m_actionInstance = nullptr;
				}
				// If there was some action, we can try to keep it's equivalent
				else SetProviderActionByName(self, self->m_action.Name(), keepArgumentValues);
			}

			inline static void SetProviderActionByName(ProvidedInstance* self, const std::string_view& actionName, bool keepArgumentValues) {
				const Reference<Provider> curProvider = self->ActionProvider();
				SerializedAction discoveredAction;
				Reference<Instance> discoveredActionInstance;
				if (curProvider != nullptr) {
					Reference<BaseConcreteInstance> curActionInstance = self->m_actionInstance;
					auto inspectAction = [&](const SerializedAction& action) {
						if (discoveredActionInstance != nullptr)
							return; // Ignore if we already found the action
						if (action.Name() != actionName)
							return; // Ignore if the action name is different
						// Create new instance:
						discoveredAction = action;
						discoveredActionInstance = discoveredAction.CreateInstance();
						assert(discoveredActionInstance != nullptr);
						// Copy old argument values if required:
						if (keepArgumentValues && curActionInstance != nullptr)
							curActionInstance->CopyArgumentValues(discoveredActionInstance);
					};
					curProvider->GetSerializedActions(Callback<SerializedAction>::FromCall(&inspectAction));
				}
				self->m_action = discoveredAction;
				self->m_actionInstance = discoveredActionInstance;
			}

			inline static void GetProvidedInstanceFields(ProvidedInstance* self, Callback<SerializedObject> recordElement) {
				Reference<Provider> provider = self->ActionProvider();
				// Serialize the provider:
				{
					static const Reference<const Serialization::ItemSerializer::Of<Reference<Provider>>> serializer =
						DefaultSerializer<Reference<Provider>>::Create("Object", "Action-Provider object");
					assert(serializer != nullptr);
					recordElement(serializer->Serialize(provider));
					self->SetActionProvider(provider, false, true);
				}

				// No need to continue, if the provider is missing:
				{
					provider = self->ActionProvider();
					if (provider == nullptr)
						return;
				}

				// Serialize the function-id:
				{
					std::string actionName = self->ActionName();
					static const Reference<const Serialization::ItemSerializer::Of<std::string>> serializer = 
						DefaultSerializer<std::string>::Create("Action Name", "Action name, used as the identifier within the provider-object");
					assert(serializer != nullptr);
					// TODO: [maybe] find a way to create a dynamic dropdown for available actions...
					recordElement(serializer->Serialize(actionName));
					if (actionName != self->ActionName())
						self->SetActionByName(actionName, true);
				}

				// Expose the instance-arguments if present:
				{
					const Reference<Instance> instance = self->m_actionInstance;
					if (instance != nullptr)
						instance->GetFields(recordElement);
				}
			}
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
		static_assert(std::is_assignable_v<std::string_view, std::string>);
		static_assert(std::is_assignable_v<std::string_view, const char*>);
		static_assert(std::is_assignable_v<std::string_view, const std::string_view>);
		static_assert(std::is_same_v<std::remove_reference_t<int*>, int*>);
		static_assert(std::is_same_v<std::remove_reference_t<int*&>, int*>);
		static_assert(std::is_same_v<std::remove_reference_t<const int*>, const int*>);

		/// <summary>
		/// Creates SerializedAction of given name with given underlying function and the argument names or serializers;
		/// <para/> If argSerializers has more entries than the number of arguments the function expects, the tail-end of the sequence will be ignored;
		/// <para/> If argSerializers has less entries than the number of arguments the function expects, the remainder of the arguments will be assigned unnamed serializers;
		/// <para/> If argSerializers contains nulls, the effected arguments will not be serialized.
		/// </summary>
		/// <typeparam name="...ArgSerializers"> A list of argument serializer references or just argument names for internal serializer-creation </typeparam>
		/// <param name="name"> Function name </param>
		/// <param name="action"> Underlying action to use </param>
		/// <param name="...argSerializers"> Serializers and/or argument names and/or FieldInfo entries; in the order of the arguments </param>
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
			using ArgListT = typename Helpers::ArgList<Args...>;
			ArgListT::CollectSerializers(result.m_argumentSerializers, argSerializers...);

			result.m_createInstance = [](const SerializedAction* act) -> Reference<Instance> {
				using InstanceType = typename Helpers::ConcreteInstance<Args...>;
				Reference<InstanceType> instance = Object::Instantiate<InstanceType>();
				instance->action = *reinterpret_cast<const Function<ReturnType, Args...>*>(reinterpret_cast<const void*>(&act->m_baseAction));
				instance->arguments.FillSerializers(0u, act->m_argumentSerializers);
				return instance;
			};

			return result;
		}
	}
}
