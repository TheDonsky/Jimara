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
