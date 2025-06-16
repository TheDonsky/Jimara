#pragma once
#include "ItemSerializers.h"
#include "../../Core/WeakReference.h"
#include <sstream>


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Template for defining default serializers for custom types
		/// <para/> Note: Works by default for any type that has a nested type called 'Serializer'; 
		///		Value types have their own overrides and feel free to add more if you need to.
		/// </summary>
		/// <typeparam name="TargetType"></typeparam>
		template<typename TargetType>
		struct DefaultSerializer {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef typename TargetType::Serializer Serializer_T;

			/// <summary>
			/// Creates a default serializer for any class that has a subtype called 'Serilizer'
			/// </summary>
			/// <typeparam name="TargetType"> Type of the target object </typeparam>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Object::Instantiate<Serializer_T>(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for bool
		/// </summary>
		template<>
		struct DefaultSerializer<bool> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<bool>::From<bool> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of bool reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<bool>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for char
		/// </summary>
		template<>
		struct DefaultSerializer<char> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<char>::From<char> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of char reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<char>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for signed char
		/// </summary>
		template<>
		struct DefaultSerializer<signed char> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<signed char>::From<signed char> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of signed char reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<signed char>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for unsigned char
		/// </summary>
		template<>
		struct DefaultSerializer<unsigned char> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<unsigned char>::From<unsigned char> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of unsigned char reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<unsigned char>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for wchar_t
		/// </summary>
		template<>
		struct DefaultSerializer<wchar_t> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<wchar_t>::From<wchar_t> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of wchar_t reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<wchar_t>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for short
		/// </summary>
		template<>
		struct DefaultSerializer<short> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<short>::From<short> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of short reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<short>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for unsigned short
		/// </summary>
		template<>
		struct DefaultSerializer<unsigned short> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<unsigned short>::From<unsigned short> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of unsigned short reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<unsigned short>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for int
		/// </summary>
		template<>
		struct DefaultSerializer<int> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<int>::From<int> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of int reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<int>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for unsigned int
		/// </summary>
		template<>
		struct DefaultSerializer<unsigned int> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<unsigned int>::From<unsigned int> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of unsigned int reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<unsigned int>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for long
		/// </summary>
		template<>
		struct DefaultSerializer<long> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<long>::From<long> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of long reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<long>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for unsigned long
		/// </summary>
		template<>
		struct DefaultSerializer<unsigned long> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<unsigned long>::From<unsigned long> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of unsigned long reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<unsigned long>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for long long
		/// </summary>
		template<>
		struct DefaultSerializer<long long> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<long long>::From<long long> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of long long reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<long long>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for unsigned long long
		/// </summary>
		template<>
		struct DefaultSerializer<unsigned long long> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<unsigned long long>::From<unsigned long long> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of unsigned long long reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<unsigned long long>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for float
		/// </summary>
		template<>
		struct DefaultSerializer<float> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<float>::From<float> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of float reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<float>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for double
		/// </summary>
		template<>
		struct DefaultSerializer<double> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<double>::From<double> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of double reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<double>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Vector2
		/// </summary>
		template<>
		struct DefaultSerializer<Vector2> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Vector2>::From<Vector2> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Vector2 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Vector2>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Vector3
		/// </summary>
		template<>
		struct DefaultSerializer<Vector3> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Vector3>::From<Vector3> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Vector3 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Vector3>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Vector4
		/// </summary>
		template<>
		struct DefaultSerializer<Vector4> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Vector4>::From<Vector4> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Vector4 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Vector4>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Matrix2
		/// </summary>
		template<>
		struct DefaultSerializer<Matrix2> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Matrix2>::From<Matrix2> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Matrix2 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Matrix2>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Matrix3
		/// </summary>
		template<>
		struct DefaultSerializer<Matrix3> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Matrix3>::From<Matrix3> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Matrix3 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Matrix3>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for Matrix4
		/// </summary>
		template<>
		struct DefaultSerializer<Matrix4> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<Matrix4>::From<Matrix4> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Matrix4 reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Matrix4>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for std::string_view
		/// </summary>
		template<>
		struct DefaultSerializer<std::string_view> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<std::string_view>::From<std::string_view> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of std::string_view reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<std::string_view>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for std::string
		/// </summary>
		template<>
		struct DefaultSerializer<std::string> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<std::string_view>::From<std::string> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of std::string reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				typedef std::string_view(*GetFn)(std::string*);
				typedef void(*SetFn)(const std::string_view&, std::string*);
				return Serialization::ValueSerializer<std::string_view>::Create<std::string>(name, hint,
					(GetFn)[](std::string* target) -> std::string_view { return *target; },
					(SetFn)[](const std::string_view& value, std::string* target) { (*target) = value; },
					attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for std::wstring_view
		/// </summary>
		template<>
		struct DefaultSerializer<std::wstring_view> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<std::wstring_view>::From<std::wstring_view> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of std::wstring_view reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<std::wstring_view>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for std::wstring
		/// </summary>
		template<>
		struct DefaultSerializer<std::wstring> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef Serialization::ValueSerializer<std::wstring_view>::From<std::wstring> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of std::wstring reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				typedef std::wstring_view(*GetFn)(std::wstring*);
				typedef void(*SetFn)(const std::wstring_view&, std::wstring*);
				return Serialization::ValueSerializer<std::wstring_view>::Create<std::wstring>(name, hint,
					(GetFn)[](std::wstring* target) -> std::wstring_view { return *target; },
					(SetFn)[](const std::wstring_view& value, std::wstring* target) { (*target) = value; },
					attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for pointer types
		/// </summary>
		template<typename ReferencedType>
		struct DefaultSerializer<ReferencedType*> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef typename Serialization::ValueSerializer<Reference<ReferencedType>>::template From<ReferencedType*> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of ReferencedType* reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				typedef Reference<ReferencedType>(*GetFn)(ReferencedType**);
				typedef void (*SetFn)(const Reference<ReferencedType>&, ReferencedType**);
				return Serialization::ValueSerializer<Reference<ReferencedType>>::template Create<ReferencedType*>(
					name, hint, 
					(GetFn)[](ReferencedType** target)->Reference<ReferencedType> { return *target; },
					(SetFn)[](const Reference<ReferencedType>& value, ReferencedType** target) { (*target) = value; },
					attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for reference types
		/// </summary>
		template<typename ReferencedType>
		struct DefaultSerializer<Reference<ReferencedType>> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef typename Serialization::ValueSerializer<Reference<ReferencedType>>::template From<Reference<ReferencedType>> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of Reference&lt;ReferencedType&gt;
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Serialization::ValueSerializer<Reference<ReferencedType>>::Create(name, hint, attributes);
			}
		};

		/// <summary>
		/// ValueSerializer creator for weak reference types
		/// </summary>
		template<typename ReferencedType>
		struct DefaultSerializer<WeakReference<ReferencedType>> {
			/// <summary> Type of the Serializer, created by this struct </summary>
			typedef typename Serialization::ValueSerializer<Reference<ReferencedType>>::template From<WeakReference<ReferencedType>> Serializer_T;

			/// <summary>
			/// Creates ValueSerializer of WeakReference<ReferencedType> reference
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				typedef Reference<ReferencedType>(*GetFn)(WeakReference<ReferencedType>*);
				typedef void (*SetFn)(const Reference<ReferencedType>&, WeakReference<ReferencedType>*);
				return Serialization::ValueSerializer<Reference<ReferencedType>>::template Create<WeakReference<ReferencedType>>(
					name, hint,
					(GetFn)[](WeakReference<ReferencedType>* target)->Reference<ReferencedType> { return *target; },
					(SetFn)[](const Reference<ReferencedType>& value, WeakReference<ReferencedType>* target) { (*target) = value; },
					attributes);
			}
		};

		/// <summary>
		/// Default serializer for std::vector types
		/// <para/> Notes:
		/// <para/>		0. Keep in mind that this only works if stored elements have their default serializers;
		/// <para/>		1. Element serializers will not "inherit" attributes; provide custom wrapper with custom default serializer for underlying elements to insert those.
		/// </summary>
		/// <typeparam name="Type"> Vector element type </typeparam>
		template<typename Type>
		struct DefaultSerializer<std::vector<Type>> {
			struct Serializer_T : public virtual Serialization::SerializerList::From<std::vector<Type>> {
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="name"> Name of the ItemSerializer </param>
				/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
				/// <param name="attributes"> Serializer attributes </param>
				inline Serializer_T(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
					: Serialization::ItemSerializer(name, hint, attributes) {}

				/// <summary> Virtual destructor </summary>
				inline virtual ~Serializer_T() {}

				/// <summary>
				/// Serializes fields of the list
				/// </summary>
				/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
				/// <param name="targetAddr"> Serializer target object </param>
				inline virtual void GetFields(const Callback<SerializedObject>& recordElement, std::vector<Type>* target)const override {
					if (target == nullptr)
						return;
					static const auto sizeSerializer = DefaultSerializer<size_t>::Create("Size", "Number of elements");
					size_t size = target->size();
					recordElement(sizeSerializer->Serialize(size));
					if (target->size() != size)
						target->resize(size);
					static thread_local auto firstElementSerializer = DefaultSerializer<Type>::Create("0", "Element at index 0", {});
					using Serializer_T = decltype(firstElementSerializer);
					static thread_local std::vector<Serializer_T> elementSerializers = { firstElementSerializer };
					while (elementSerializers.size() < size) {
						std::stringstream stream;
						stream << elementSerializers.size();
						const std::string name = stream.str();
						const std::string hint = "Element at index " + name;
						elementSerializers.push_back(DefaultSerializer<Type>::Create(name, hint, {}));
					}
					for (size_t i = 0u; i < Math::Min(size, target->size()); i++)
						recordElement(elementSerializers[i]->Serialize(target->data() + i));
				}
			};

			/// <summary>
			/// Creates ValueSerializer of std::vector<Type>
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			/// <returns> Serializer instance </returns>
			inline static Reference<const Serializer_T> Create(
				const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {}) {
				return Object::Instantiate<Serializer_T>(name, hint, attributes);
			}
		};
	}
}
