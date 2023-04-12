#pragma once
#include <functional>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace Jimara {
	/// <summary>
	/// Arbirtary method/function pointer (Takes object pointer alongside method (member function); does not guarantee any safety when the object gets destroyed)
	/// </summary>
	/// <typeparam name="ReturnType"> Function return value type </typeparam>
	/// <typeparam name="...Args"> Function/Method arguments </typeparam>
	template<typename ReturnType, typename... Args>
	class Function {
	public:
		/// <summary>
		/// Constructs from non-member function
		/// </summary>
		/// <param name="function"> non-member function </param>
		inline Function(ReturnType(*function)(Args...))
			: m_object(nullptr), m_caller(CallFunction) {
			memset(&m_function, 0, sizeof(m_function));
			reinterpret_cast<FunctionPtr*>(&m_function)->function = function;
		}

		/// <summary>
		/// Constructs from a member function pointer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Function(ReturnType(ObjectType::* method)(Args...), ObjectType* object)
			: m_object((void*)object), m_caller(CallMethod<ObjectType>) {
			static_assert(sizeof(FunctionStorage) >= sizeof(MethodPtr<ObjectType>));
			memset(&m_function, 0, sizeof(m_function));
			reinterpret_cast<MethodPtr<ObjectType>*>(&m_function)->method = method;
		}

		/// <summary>
		/// Constructs from a member function pointer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Function(ReturnType(ObjectType::* method)(Args...), ObjectType& object) : Function(method, &object) { }

		/// <summary>
		/// Constructs from a member function pointer (const)
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Function(ReturnType(ObjectType::* method)(Args...)const, const ObjectType* object)
			: m_object((void*)object), m_caller(CallConstMethod<const ObjectType>) {
			static_assert(sizeof(FunctionStorage) >= sizeof(ConstMethodPtr<ObjectType>));
			memset(&m_function, 0, sizeof(m_function));
			reinterpret_cast<ConstMethodPtr<ObjectType>*>(&m_function)->method = method;
		}

		/// <summary>
		/// Constructs from a member function pointer (const)
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Function(ReturnType(ObjectType::* method)(Args...)const, const ObjectType& object) : Function(method, &object) { }

		/// <summary>
		/// Constructs from a non-member function pointer and user data it gets as the first parameter
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the user data </typeparam>
		/// <param name="function"> Function pointer </param>
		/// <param name="userData"> User data </param>
		template<typename ObjectType>
		inline Function(ReturnType(*function)(ObjectType*, Args...), ObjectType* userData) 
			: m_object((void*)userData), m_caller(CallWithUserDataPtr<ObjectType>) {
			static_assert(sizeof(FunctionStorage) >= sizeof(FunctionWithUserDataPtr<ObjectType>));
			memset(&m_function, 0, sizeof(m_function));
			reinterpret_cast<FunctionWithUserDataPtr<ObjectType>*>(&m_function)->function = function;
		}

		/// <summary>
		/// Constructs from a non-member function pointer and const user data it gets as the first parameter
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the user data </typeparam>
		/// <param name="function"> Function pointer </param>
		/// <param name="userData"> User data </param>
		template<typename ObjectType>
		inline Function(ReturnType(*function)(const ObjectType*, Args...), const ObjectType* userData)
			: m_object((void*)userData), m_caller(CallWithConstUserDataPtr<ObjectType>) {
			static_assert(sizeof(FunctionStorage) >= sizeof(FunctionWithConstUserDataPtr<ObjectType>));
			memset(&m_function, 0, sizeof(m_function));
			reinterpret_cast<FunctionWithConstUserDataPtr<ObjectType>*>(&m_function)->function = function;
		}

		/// <summary>
		/// Creates a function from an arbitarary object for which it's valid to invoke (*callable)(args...);
		/// <para /> Note: Lifecylce of the callable object should be taken care of externally; 
		///		temporary lambdas with long-lived references will cause a lot of trouble.
		/// </summary>
		/// <typeparam name="Callable"> Lambda, or any class with "ReturnType operator()(Args...)" </typeparam>
		/// <param name="callable"> Pointer to anything that can be invoked as a function </param>
		/// <returns> Wrapper to the call </returns>
		template<typename Callable>
		inline static Function FromCall(Callable* callable) {
			typedef ReturnType(*InvokeFn)(Callable*, Args...);
			static const InvokeFn invokeFn = [](Callable* addr, Args... args) -> ReturnType { return (*addr)(args...); };
			return Function(invokeFn, callable);
		}

		/// <summary>
		/// Same as (this) = Function::FromCall(callable)
		/// </summary>
		/// <typeparam name="Callable"> Lambda, or any class with "ReturnType operator()(Args...)" </typeparam>
		/// <param name="callable"> Pointer to anything that can be invoked as a function </param>
		/// <returns> Self </returns>
		template<typename Callable>
		inline Function& operator=(Callable* callable) {
			(*this) = Function::FromCall(callable);
			return (*this);
		}

		/// <summary>
		/// Invokes underlying function
		/// </summary>
		/// <param name="...args"> Arguments to invoke function with </param>
		/// <returns> Whatever the underlying function would return </returns>
		inline ReturnType operator()(Args... args)const { return m_caller(this, args...); }

		/// <summary> (this is 'less than' other) comparator </summary>
		inline bool operator<(const Function& other)const {
			return (m_object < other.m_object) || (m_object == other.m_object && (memcmp(&m_function, &other.m_function, sizeof(m_function)) < 0));
		}

		/// <summary> (this is 'less than or equal to' other) comparator </summary>
		inline bool operator<=(const Function& other)const {
			return (m_object < other.m_object) || (m_object == other.m_object && (memcmp(&m_function, &other.m_function, sizeof(m_function)) <= 0));
		}

		/// <summary> (this is 'equal to' other) comparator </summary>
		inline bool operator==(const Function& other)const {
			return (m_object == other.m_object && (memcmp(&m_function, &other.m_function, sizeof(m_function)) == 0));
		}

		/// <summary> (this is 'not equal to' other) comparator </summary>
		inline bool operator!=(const Function& other)const { return !((*this) == other); }

		/// <summary> (this is 'greater than or equal to' other) comparator </summary>
		inline bool operator>=(const Function& other)const { return other <= (*this); }

		/// <summary> (this is 'greater than' other) comparator </summary>
		inline bool operator>(const Function& other)const { return other < (*this); }

		/// <summary> Function hasher for unordered collections </summary>
		struct Hash {
			/// <summary>
			/// Calculates hash
			/// </summary>
			/// <param name="func"> Function to count hash for </param>
			/// <returns> Hash </returns>
			inline size_t operator()(const Function& func)const {
				size_t hash = std::hash<void*>()(func.m_object);
				const char* ptr = func.m_function.data;
				while ((sizeof(FunctionStorage) - (ptr - func.m_function.data)) >= sizeof(uint64_t)) {
					hash ^= std::hash<uint64_t>()(*reinterpret_cast<const uint64_t*>(ptr));
					ptr += sizeof(uint64_t);
				}
				while ((sizeof(FunctionStorage) - (ptr - func.m_function.data)) >= sizeof(uint32_t)) {
					hash ^= std::hash<uint32_t>()(*reinterpret_cast<const uint32_t*>(ptr));
					ptr += sizeof(uint32_t);
				}
				while ((sizeof(FunctionStorage) - (ptr - func.m_function.data)) > 0) {
					hash ^= std::hash<char>()(*ptr);
					ptr += sizeof(char);
				}
				return hash;
			}
		};



	private:
		// Pointer to a method of an arbitrary class
		template<typename ObjectType>
		struct MethodPtr { ReturnType(ObjectType::* method)(Args...); };

		// Pointer to a method of an arbitrary immutable class
		template<typename ObjectType>
		struct ConstMethodPtr { ReturnType(ObjectType::* method)(Args...)const; };

		// Pointer to a function with an arbitrary user data pointer
		template<typename ObjectType>
		struct FunctionWithUserDataPtr { ReturnType(*function)(ObjectType*, Args...); };

		// Pointer to a function with an arbitrary immutable user data pointer
		template<typename ObjectType>
		struct FunctionWithConstUserDataPtr { ReturnType(*function)(const ObjectType*, Args...); };

		// Pointer to a non-member function
		union FunctionPtr { ReturnType(*function)(Args...); };

		// Function pointer (in actuality, memory can be occupied either by FunctionPtr, MethodPtr or ConstMethodPtr)
		struct FunctionStorage { char data[(sizeof(FunctionPtr) > sizeof(MethodPtr<Function>) ? sizeof(FunctionPtr) : sizeof(MethodPtr<Function>)) << 2]; };
		FunctionStorage m_function;

		// Object to call method for
		void* m_object;

		// Function caller
		ReturnType(*m_caller)(const Function*, Args...);

		// Calls m_function as FunctionPtr
		inline static ReturnType CallFunction(const Function* function, Args... args) {
			return reinterpret_cast<const FunctionPtr*>(&function->m_function)->function(args...);
		}

		// Calls m_function as MethodPtr
		template<typename ObjectType>
		inline static ReturnType CallMethod(const Function* function, Args... args) {
			return (((ObjectType*)function->m_object)->*reinterpret_cast<const MethodPtr<ObjectType>*>(&function->m_function)->method)(args...);
		}

		// Calls m_function as ConstMethodPtr
		template<typename ObjectType>
		inline static ReturnType CallConstMethod(const Function* function, Args... args) {
			return (((const ObjectType*)function->m_object)->*reinterpret_cast<const ConstMethodPtr<ObjectType>*>(&function->m_function)->method)(args...);
		}

		// Calls m_function as FunctionWithUserDataPtr
		template<typename ObjectType>
		inline static ReturnType CallWithUserDataPtr(const Function* function, Args... args) {
			return reinterpret_cast<const FunctionWithUserDataPtr<ObjectType>*>(&function->m_function)->function((ObjectType*)function->m_object, args...);
		}

		// Calls m_function as FunctionWithConstUserDataPtr
		template<typename ObjectType>
		inline static ReturnType CallWithConstUserDataPtr(const Function* function, Args... args) {
			return reinterpret_cast<const FunctionWithConstUserDataPtr<ObjectType>*>(&function->m_function)->function((const ObjectType*)function->m_object, args...);
		}
	};



	/// <summary>
	/// Short for Function<void, Args...>
	/// </summary>
	/// <typeparam name="...Args"> Function argument types </typeparam>
	template<typename... Args>
	class Callback : public Function<void, Args...> {
	public:
		/// <summary>
		/// Constructs from non-member function
		/// </summary>
		/// <param name="function"> non-member function </param>
		inline Callback(void(*function)(Args...)) : Function<void, Args...>(function) { }

		/// <summary>
		/// Constructs from a member function pointer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Callback(void(ObjectType::* method)(Args...), ObjectType* object) : Function<void, Args...>(method, object) { }

		/// <summary>
		/// Constructs from a member function pointer
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Callback(void(ObjectType::* method)(Args...), ObjectType& object) : Function<void, Args...>(method, object) { }

		/// <summary>
		/// Constructs from a member function pointer (const)
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Callback(void(ObjectType::* method)(Args...)const, const ObjectType* object) : Function<void, Args...>(method, object) { }


		/// <summary>
		/// Constructs from a member function pointer (const)
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the object </typeparam>
		/// <param name="method"> member function pointer </param>
		/// <param name="object"> Object to call the method for </param>
		template<typename ObjectType>
		inline Callback(void(ObjectType::* method)(Args...)const, const ObjectType& object) : Function<void, Args...>(method, object) { }

		/// <summary>
		/// Constructs from a non-member function pointer and user data it gets as the first parameter
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the user data </typeparam>
		/// <param name="function"> Function pointer </param>
		/// <param name="userData"> User data </param>
		template<typename ObjectType>
		inline Callback(void(*function)(ObjectType*, Args...), ObjectType* userData) : Function<void, Args...>(function, userData) {}

		/// <summary>
		/// Constructs from a non-member function pointer and const user data it gets as the first parameter
		/// </summary>
		/// <typeparam name="ObjectType"> Type of the user data </typeparam>
		/// <param name="function"> Function pointer </param>
		/// <param name="userData"> User data </param>
		template<typename ObjectType>
		inline Callback(void(*function)(const ObjectType*, Args...), const ObjectType* userData) : Function<void, Args...>(function, userData) {}

		/// <summary>
		/// Creates a callback from an arbitarary object for which it's valid to invoke (*callable)(args...);
		/// <para /> Note: Lifecylce of the callable object should be taken care of externally; 
		///		temporary lambdas with long-lived references will cause a lot of trouble.
		/// </summary>
		/// <typeparam name="Callable"> Lambda, or any class with "AnyType operator()(Args...)" </typeparam>
		/// <param name="callable"> Pointer to anything that can be invoked as a function </param>
		/// <returns> Wrapper to the call </returns>
		template<typename Callable>
		inline static Callback FromCall(Callable* callable) {
			typedef void(*InvokeFn)(Callable*, Args...);
			static const InvokeFn invokeFn = [](Callable* addr, Args... args) { (*addr)(args...); };
			return Callback(invokeFn, callable);
		}
	};
}



namespace std {
	/// <summary> std::hash<> override for Function<ReturnType, Args...> </summary>
	template<typename ReturnType, typename... Args>
	struct hash<Jimara::Function<ReturnType, Args...>> : public Jimara::Function<ReturnType, Args...>::Hash { };

	/// <summary> std::hash<> override for Callback<Args...> </summary>
	template<typename... Args>
	struct hash<Jimara::Callback<Args...>> : public Jimara::Callback<Args...>::Hash { };
}
