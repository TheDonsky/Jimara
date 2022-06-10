#pragma once
#include "Serialization/ItemSerializers.h"
#include <functional>
#include <iostream>
#include <cstdint>


namespace Jimara {
	/// <summary>
	/// Universally unique identifier
	/// </summary>
	struct GUID {
		// Well... we can't trust anything these days
		static_assert(sizeof(uint8_t) == 1);

		/// <summary> Number of bytes per GUID </summary>
		static const size_t NUM_BYTES = 32;

		/// <summary> GUID data </summary>
		uint8_t bytes[NUM_BYTES];

		/// <summary> Generates a new GUID </summary>
		static GUID Generate();

		/// <summary> Empty/Null GUID (basically, GUID with all bytes set to 0) </summary>
		static GUID Null();

		/// <summary> Compares with other GUID (returns true, if this is alphanumerically 'less' than the other) </summary>
		bool operator<(const GUID& other)const;

		/// <summary> Compares with other GUID (returns true, if this is alphanumerically 'less' than or equal to the other) </summary>
		bool operator<=(const GUID& other)const;
		
		/// <summary> Compares with other GUID (returns true, if this is equal to the other) </summary>
		bool operator==(const GUID& other)const;

		/// <summary> Compares with other GUID (returns true, if this is not equal to the other) </summary>
		bool operator!=(const GUID& other)const;

		/// <summary> Compares with other GUID (returns true, if this is alphanumerically 'greater' than or equal to the other) </summary>
		bool operator>=(const GUID& other)const;

		/// <summary> Compares with other GUID (returns true, if this is alphanumerically 'greater' than the other) </summary>
		bool operator>(const GUID& other)const;

		/// <summary>
		/// "Standard" Serializer for GUIDs
		/// </summary>
		class Serializer : public virtual Serialization::SerializerList::From<GUID> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Name of the GUID field </param>
			/// <param name="hint"> Feild description </param>
			/// <param name="attributes"> Item serializer attribute list </param>
			Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {});

			/// <summary>
			/// Gives access to sub-serializers/fields
			/// </summary>
			/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
			/// <param name="targetAddr"> Serializer target object </param>
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, GUID* target)const final override;
		};
	};


	/// <summary>
	/// Outputs GUID to stream
	/// </summary>
	/// <param name="stream"> std::ostream to output to </param>
	/// <param name="guid"> GUID to serialize </param>
	/// <returns> stream </returns>
	std::ostream& operator<<(std::ostream& stream, const GUID& guid);

	/// <summary>
	/// Reads GUID from a stream
	/// </summary>
	/// <param name="stream"> std::istream to read from </param>
	/// <param name="guid"> GUID to read into </param>
	/// <returns> stream </returns>
	std::istream& operator>>(std::istream& stream, GUID& guid);
}

namespace std {
	/// <summary>
	/// std::hash, overloaded for GUIDs
	/// </summary>
	template<>
	struct hash<Jimara::GUID> {
		/// <summary>
		/// Hash function for GUIDs
		/// </summary>
		/// <param name="guid"> GUID to hash </param>
		/// <returns> Hashed id </returns>
		std::size_t operator()(const Jimara::GUID& guid)const;
	};
}
