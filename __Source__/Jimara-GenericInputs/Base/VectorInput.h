#pragma once
#include "../Types.h"
#include <Jimara/Math/Math.h>
#include <Jimara/Environment/Scene/Scene.h>
#include <Jimara/Core/Systems/InputProvider.h>


namespace Jimara {
	namespace VectorInput {
		/// <summary>
		/// Generic input from given type that can be referenced as an input of bool, float and Vector2/3/4 types
		/// <para/> To create a custom VectorInput, one needs to implement VectorInput::From&lt;Type&gt; and override EvaluateInput method;
		/// </summary>
		/// <typeparam name="Type"> Input value type </typeparam>
		/// <typeparam name="...Args"> Arguments for evaluation </typeparam>
		template<typename Type, typename... Args>
		class From;

		/// <summary>
		/// Tells of we can have an implementation of VectorInput::From&lt;Type&gt;
		/// <para/> Returns true for bool, float and Vector2/3/4 types
		/// </summary>
		/// <typeparam name="Type"> Input value type </typeparam>
		template<typename Type>
		inline static constexpr bool IsCompatibleType = (
			std::is_same_v<Type, bool> ||
			std::is_same_v<Type, float> ||
			std::is_same_v<Type, Vector2> ||
			std::is_same_v<Type, Vector3> ||
			std::is_same_v<Type, Vector4>);

		/// <summary>
		/// Base class encapsulating the EvaluateInput generic method
		/// <para/> User is free to ignore this one... Implementation of From simply inherits from this
		/// </summary>
		/// <typeparam name="Type"></typeparam>
		/// <typeparam name="...Args"></typeparam>
		template<typename Type, typename... Args>
		class VectorInput_Base {
		public:
			/// <summary> Virtual Destructor </summary>
			inline virtual ~VectorInput_Base() {}

			/// <summary>
			/// Provides "input" value;
			/// <para/> Return type is optional and, therefore, the input is allowed to be empty.
			/// </summary>
			/// <param name="...args"> Some contextual arguments </param>
			/// <returns> Input value if present </returns>
			virtual std::optional<Type> EvaluateInput(Args... args) = 0;
		};

		/// <summary>
		/// Base class for InputProvider implementation
		/// <para/> User is free to ignore this one... Implementation of From simply inherits from this
		/// </summary>
		/// <typeparam name="Type"> Base value type </typeparam>
		/// <typeparam name="InputType"> Input provider type </typeparam>
		/// <typeparam name="...Args"> Evaluation Arguments </typeparam>
		template<typename Type, typename InputType, typename... Args>
		class VectorInput_InputBase
			: public virtual VectorInput_Base<Type, Args...>
			, public virtual InputProvider<InputType> {
		public:
			/// <summary>
			/// 'Casts' base input, evaluated via EvaluateInput to InputType and returns it
			/// </summary>
			/// <param name="...args"> Evaluation arguments </param>
			/// <returns> Type-casted value </returns>
			inline virtual std::optional<InputType> GetInput(Args... args)final override {
				const std::optional<Type> intermediateResult = this->EvaluateInput(args...);
				if (intermediateResult.has_value()) {
					InputType result = {};
					Helpers::Cast(intermediateResult.value(), result);
					return result;
				}
				else return std::optional<InputType>();
			}

		private:
			// Only 'From' can access the implementation
			inline VectorInput_InputBase() {}
			friend class From<Type, Args...>;

			// Private type-cast functions reside in here
			struct Helpers;
		};


		/// <summary>
		/// Generic input from given type that can be referenced as an input of bool, float and Vector2/3/4 types
		/// <para/> To create a custom VectorInput, one needs to implement VectorInput::From&lt;Type&gt; and override EvaluateInput method;
		/// </summary>
		/// <typeparam name="Type"> Input value type </typeparam>
		/// <typeparam name="...Args"> Arguments for evaluation </typeparam>
		template<typename Type, typename... Args>
		class From
			: public VectorInput_InputBase<Type, bool, Args...>
			, public VectorInput_InputBase<Type, float, Args...>
			, public VectorInput_InputBase<Type, Vector2, Args...>
			, public VectorInput_InputBase<Type, Vector3, Args...>
			, public VectorInput_InputBase<Type, Vector4, Args...> {
		public:
			/// <summary> Constructor </summary>
			inline From() {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~From() {}
		};


		/// <summary>
		/// Vector input provider that is also a Component
		/// </summary>
		/// <typeparam name="Type"> Input value type </typeparam>
		/// <typeparam name="...Args"> Arguments for evaluation </typeparam>
		template<typename Type, typename... Args>
		class ComponentFrom
			: public virtual Component
			, public virtual From<Type, Args...> {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~ComponentFrom() = 0;

			/// <summary>
			/// [Only intended to be used by WeakReference<>; not safe for general use] Fills WeakReferenceHolder with a StrongReferenceProvider, 
			/// that will return this WeaklyReferenceable back upon request (as long as it still exists)
			/// <para/> Note that this is not thread-safe!
			/// </summary>
			/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
			inline virtual void FillWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
				Component::FillWeakReferenceHolder(holder);
			}

			/// <summary>
			/// [Only intended to be used by WeakReference<>; not safe for general use] Should clear the link to the StrongReferenceProvider;
			/// <para/> Address of the holder has to be the same as the one, previously passed to FillWeakReferenceHolder() method
			/// <para/> Note that this is not thread-safe!
			/// </summary>
			/// <param name="holder"> Reference to a reference of a StrongReferenceProvider </param>
			inline virtual void ClearWeakReferenceHolder(WeaklyReferenceable::WeakReferenceHolder& holder)override {
				Component::ClearWeakReferenceHolder(holder);
			}
		};

		/// <summary> Virtual destructor </summary>
		template<typename Type, typename... Args>
		inline ComponentFrom<Type, Args...>::~ComponentFrom() {}





		// Type-cast operations
		template<typename Type, typename InputType, typename... Args>
		struct VectorInput_InputBase<Type, InputType, Args...>::Helpers {
			inline static void Cast(bool src, bool& dst) { dst = src; }
			inline static void Cast(bool src, float& dst) { dst = float(src); }
			inline static void Cast(bool src, Vector2& dst) { dst = Vector2(float(src), 0.0f); }
			inline static void Cast(bool src, Vector3& dst) { dst = Vector3(float(src), 0.0f, 0.0f); }
			inline static void Cast(bool src, Vector4& dst) { dst = Vector4(float(src), 0.0f, 0.0f, 0.0f); }

			inline static void Cast(float src, bool& dst) { dst = (src != 0.0f); }
			inline static void Cast(float src, float& dst) { dst = src; }
			inline static void Cast(float src, Vector2& dst) { dst = Vector2(src, 0.0f); }
			inline static void Cast(float src, Vector3& dst) { dst = Vector3(src, 0.0f, 0.0f); }
			inline static void Cast(float src, Vector4& dst) { dst = Vector4(src, 0.0f, 0.0f, 0.0f); }

			inline static void Cast(const Vector2& src, bool& dst) { dst = (Math::SqrMagnitude(src) > 0.0f); }
			inline static void Cast(const Vector2& src, float& dst) { dst = src.x; }
			inline static void Cast(const Vector2& src, Vector2& dst) { dst = src; }
			inline static void Cast(const Vector2& src, Vector3& dst) { dst = Vector3(src.x, src.y, 0.0f); }
			inline static void Cast(const Vector2& src, Vector4& dst) { dst = Vector4(src.x, src.y, 0.0f, 0.0f); }

			inline static void Cast(const Vector3& src, bool& dst) { dst = (Math::SqrMagnitude(src) > 0.0f); }
			inline static void Cast(const Vector3& src, float& dst) { dst = src.x; }
			inline static void Cast(const Vector3& src, Vector2& dst) { dst = Vector2(src.x, src.y); }
			inline static void Cast(const Vector3& src, Vector3& dst) { dst = src; }
			inline static void Cast(const Vector3& src, Vector4& dst) { dst = Vector4(src.x, src.y, src.z, 0.0f); }

			inline static void Cast(const Vector4& src, bool& dst) { dst = (Math::SqrMagnitude(src) > 0.0f); }
			inline static void Cast(const Vector4& src, float& dst) { dst = src.x; }
			inline static void Cast(const Vector4& src, Vector2& dst) { dst = Vector2(src.x, src.y); }
			inline static void Cast(const Vector4& src, Vector3& dst) { dst = Vector3(src.x, src.y, src.z); }
			inline static void Cast(const Vector4& src, Vector4& dst) { dst = src; }
		};
	}

	// Just sanity check
	static_assert(float(true) == 1.0f);
	static_assert(float(false) == 0.0f);

	/// <summary>
	/// Type details for VectorInput::From
	/// </summary>
	/// <typeparam name="Type"> Base value type </typeparam>
	/// <typeparam name="...Args"> Evaluation Arguments </typeparam>
	template<typename Type, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorInput::From<Type, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::VectorInput_InputBase<Type, bool, Args...>>());
			reportParentType(TypeId::Of<VectorInput::VectorInput_InputBase<Type, float, Args...>>());
			reportParentType(TypeId::Of<VectorInput::VectorInput_InputBase<Type, Vector2, Args...>>());
			reportParentType(TypeId::Of<VectorInput::VectorInput_InputBase<Type, Vector3, Args...>>());
			reportParentType(TypeId::Of<VectorInput::VectorInput_InputBase<Type, Vector4, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorInput::VectorInput_InputBase
	/// </summary>
	/// <typeparam name="Type"> Base value type </typeparam>
	/// <typeparam name="InputType"> Input provider type </typeparam>
	/// <typeparam name="...Args"> Evaluation Arguments </typeparam>
	template<typename Type, typename InputType, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorInput::VectorInput_InputBase<Type, InputType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<VectorInput::VectorInput_Base<Type, Args...>>());
			reportParentType(TypeId::Of<InputProvider<InputType>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Type details for VectorInput::ComponentFrom
	/// </summary>
	/// <typeparam name="Type"> Base value type </typeparam>
	/// <typeparam name="InputType"> Input provider type </typeparam>
	/// <typeparam name="...Args"> Evaluation Arguments </typeparam>
	template<typename Type, typename InputType, typename... Args>
	struct TypeIdDetails::TypeDetails<VectorInput::ComponentFrom<Type, InputType, Args...>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Component>());
			reportParentType(TypeId::Of<VectorInput::From<Type, Args...>>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};
}
