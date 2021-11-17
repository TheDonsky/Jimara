#pragma once
#include "../../../Core/Memory/MemoryBlock.h"
#include "../../../Core/Function.h"
#include "../../../OS/Logging/Logger.h"
#include "../../../OS/IO/Path.h"
#include "../../../Math/Math.h"
#include <ostream>


namespace Jimara {
	/// <summary>
	/// Content from an FBX file
	/// Represents the parsed Node tree, stored in a file without meaningfull interpretation.
	/// </summary>
	class FBXContent : public virtual Object {
	public:
		/// <summary>
		/// Type of a Node Property
		/// </summary>
		enum class PropertyType : uint8_t {
			/// <summary> Unknown (error) type </summary>
			UNKNOWN = 0,

			/// <summary> Boolean value </summary>
			BOOLEAN = 1,

			/// <summary> Array of booleans </summary>
			BOOLEAN_ARR = 2,

			/// <summary> 16 bit signed integer </summary>
			INT_16 = 3,

			/// <summary> 32 bit signed integer </summary>
			INT_32 = 4,

			/// <summary> 32 bit signed integer array </summary>
			INT_32_ARR = 5,

			/// <summary> 64 bit signed integer </summary>
			INT_64 = 6,

			/// <summary> 64 bit signed integer array </summary>
			INT_64_ARR = 7,

			/// <summary> 32 bit floating point </summary>
			FLOAT_32 = 8,

			/// <summary> 32 bit floating point array </summary>
			FLOAT_32_ARR = 9,

			/// <summary> 64 bit floating point </summary>
			FLOAT_64 = 10,

			/// <summary> 64 bit floating point array </summary>
			FLOAT_64_ARR = 11,

			/// <summary> String property (can contain '\0' in special cases, used Size() and cast to MemoryBlock for that case) </summary>
			STRING = 12,

			/// <summary> Raw binary data </summary>
			RAW_BINARY = 13,

			/// <summary> Not an actual property; just the number of elements within the enumeration </summary>
			PROPERTY_TYPE_COUNT = 14
		};

		/// <summary>
		/// Node property
		/// Note: Keep in mind that Property becomes invalid once the owner FBXContent goes out of scope
		/// </summary>
		class Property {
		public:
			/// <summary> Property Type </summary>
			PropertyType Type()const;

			/// <summary> 
			/// Number of elements within the property 
			/// (1 for singular bool/numeric values, array size for array types, string length for String and number of bytes for raw binary data) 
			/// </summary>
			size_t Count()const;

			/// <summary> Type cast to boolean value (valid if and only if Type is BOOLEAN or BOOLEAN_ARR; same as BoolElem(0) for the later) </summary>
			operator bool()const;

			/// <summary>
			/// Array element value for BOOLEAN_ARR Type
			/// Note: BoolElem(0) is valid for BOOLEAN as well and is the same as type cast to bool
			/// </summary>
			/// <param name="index"> Array element index </param>
			/// <returns> Array element value </returns>
			bool BoolElem(size_t index)const;

			/// <summary> Type cast to 16 bit signed integer value (valid if and only if Type is INT_16) </summary>
			operator int16_t()const;

			/// <summary> Type cast to 32 bit signed integer value (valid if and only if Type is INT_32 or INT_32_ARR; same as Int32Elem(0) for the later) </summary>
			operator int32_t()const;

			/// <summary>
			/// Array element value for INT_32_ARR Type
			/// Note: Int32Elem(0) is valid for INT_32 as well and is the same as type cast to int32_t
			/// </summary>
			/// <param name="index"> Array element index </param>
			/// <returns> Array element value </returns>
			int32_t Int32Elem(size_t index)const;

			/// <summary> Type cast to 64 bit signed integer value (valid if and only if Type is INT_64 or INT_64_ARR; same as Int64Elem(0) for the later) </summary>
			operator int64_t()const;

			/// <summary>
			/// Array element value for INT_64_ARR Type
			/// Note: Int64Elem(0) is valid for INT_64 as well and is the same as type cast to int64_t
			/// </summary>
			/// <param name="index"> Array element index </param>
			/// <returns> Array element value </returns>
			int64_t Int64Elem(size_t index)const;

			/// <summary> Type cast to 32 bit floating point value (valid if and only if Type is FLOAT_32 or FLOAT_32_ARR; same as Float32Elem(0) for the later) </summary>
			operator float()const;

			/// <summary>
			/// Array element value for FLOAT_32_ARR Type
			/// Note: Float32Elem(0) is valid for FLOAT_32 as well and is the same as type cast to float
			/// </summary>
			/// <param name="index"> Array element index </param>
			/// <returns> Array element value </returns>
			float Float32Elem(size_t index)const;

			/// <summary> Type cast to 64 bit floating point value (valid if and only if Type is FLOAT_64 or FLOAT_64_ARR; same as Float64Elem(0) for the later) </summary>
			operator double()const;

			/// <summary>
			/// Array element value for FLOAT_64_ARR Type
			/// Note: Float64Elem(0) is valid for FLOAT_64 as well and is the same as type cast to float
			/// </summary>
			/// <param name="index"> Array element index </param>
			/// <returns> Array element value </returns>
			double Float64Elem(size_t index)const;

			/// <summary> Type cast to string view (valid if and only if Type is STRING) </summary>
			operator std::string_view()const;

			/// <summary> Type cast to a memory block, containing the value(s) (Valid for all Types, gives direct access to underlying buffers, c-strings and arrays) </summary>
			operator MemoryBlock()const;

			/// <summary>
			/// Gets boolean value for integer/boolean types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(bool& result)const;

			/// <summary>
			/// Gets address to boolean values if the Type is a boolean array
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(const bool*& result)const;

			/// <summary>
			/// Gets 16 bit integer value for integer/boolean types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(int16_t& result)const;

			/// <summary>
			/// Gets 32 bit integer value for integer/boolean types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(int32_t& result)const;

			/// <summary>
			/// Gets address to 32 bit integer values if the Type is a 32 bit integer array
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(const int32_t*& result)const;

			/// <summary>
			/// Gets 64 bit integer value for integer/boolean types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(int64_t& result)const;

			/// <summary>
			/// Gets address to 64 bit integer values if the Type is a 64 bit integer array
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(const int64_t*& result)const;
			
			/// <summary>
			/// Gets 32 bit floating point value for floating point types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(float& result)const;

			/// <summary>
			/// Gets address to 32 bit floating point values if the Type is a 32 bit floating point array
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(const float*& result)const;

			/// <summary>
			/// Gets 64 bit floating point value for floating point types, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(double& result)const;

			/// <summary>
			/// Gets address to 64 bit floating point values if the Type is a 64 bit floating point array
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(const double*& result)const;

			/// <summary>
			/// Gets string_view for string point type, fails otherwise
			/// </summary>
			/// <param name="result"> If the call returns true, value will be stored here </param>
			/// <returns> True, if the value gets retrieved successfully, false otherwise </returns>
			bool Get(std::string_view& result)const;

			/// <summary>
			/// Fills given buffer of vectors with data if possible (has to be a floating point array with compatible number of elements)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type or element count </returns>
			bool Fill(std::vector<Vector3>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of vectors with data if possible (has to be a floating point array with compatible number of elements)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type or element count </returns>
			bool Fill(std::vector<Vector2>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of integers with data if possible (has to be an integer or a boolean array and values can be successfully cast without underflows and/or overflows)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type or an overflow </returns>
			bool Fill(std::vector<bool>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of integers with data if possible (has to be an integer or a boolean array and values can be successfully cast without underflows and/or overflows)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type or an overflow </returns>
			bool Fill(std::vector<int32_t>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of integers with data if possible (has to be an integer or a boolean array and values can be successfully cast without underflows and/or overflows)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type or an overflow </returns>
			bool Fill(std::vector<int64_t>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of integers with data if possible (has to be an integer or a boolean array and values can be successfully cast without underflows and/or overflows)
			/// Note: if handleNegative fails, the call will fail as well and terminate with the buffer partially filled
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <param name="handleNegative"> When a negative value is encountered, this will be called with previous elements stored in the buffer; if this returns false, filling will stop and report a failure </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type, overflow, or a failed negative value handling </returns>
			bool Fill(std::vector<uint32_t>& buffer, bool clear, const Function<bool, int32_t>& handleNegative = Function<bool, int32_t>([](int32_t) ->bool { return false; }))const;

			/// <summary>
			/// Fills given buffer of integers with data if possible (has to be an integer or a boolean array and values can be successfully cast without underflows and/or overflows)
			/// Note: if handleNegative fails, the call will fail as well and terminate with the buffer partially filled
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <param name="handleNegative"> When a negative value is encountered, this will be called with previous elements stored in the buffer; if this returns false, filling will stop and report a failure </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type, overflow, or a failed negative value handling </returns>
			bool Fill(std::vector<uint64_t>& buffer, bool clear, const Function<bool, int64_t>& handleNegative = Function<bool, int64_t>([](int64_t) ->bool { return false; }))const;

			/// <summary>
			/// Fills given buffer of floating points with data if possible (has to be an floating point array)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type </returns>
			bool Fill(std::vector<float>& buffer, bool clear)const;

			/// <summary>
			/// Fills given buffer of floating points with data if possible (has to be an floating point array)
			/// </summary>
			/// <param name="buffer"> List to fill </param>
			/// <param name="clear"> If true, buffer will be cleaned before it gets filled with the content form here </param>
			/// <returns> True, if the buffer gets filled sucessfully, false if the property is incompatible due to type </returns>
			bool Fill(std::vector<double>& buffer, bool clear)const;


		private:
			// Owner
			const FBXContent* m_content = nullptr;
			
			// Type
			PropertyType m_type = PropertyType::UNKNOWN;
			
			// First element index in the m_content's value buffer corresponding to the type 
			// (ei m_rawBuffer for booleans and raw binary data, m_stringBuffer for string values, 
			// m_int16Buffer/m_int32Buffer/m_int64Buffer for signed integers, m_float32Buffer/m_float64Buffer for floating points)
			size_t m_valueOffset = 0;

			// (1 for singular bool/numeric values, array size for array types, string length for String and number of bytes for raw binary data)
			size_t m_valueCount = 0;

			// FBXContent is allowed to modify Properties (only needed during parsing, really)
			friend class FBXContent;
		};


		/// <summary>
		/// FBX Node
		/// Note: Keep in mind that Node becomes invalid once the owner FBXContent goes out of scope
		/// </summary>
		class Node {
		public:
			/// <summary> Node name </summary>
			const std::string_view Name()const;

			/// <summary> Number of node properties </summary>
			size_t PropertyCount()const;

			/// <summary>
			/// Node property by index
			/// </summary>
			/// <param name="index"> Propert index </param>
			/// <returns> index'th property </returns>
			const Property& NodeProperty(size_t index)const;

			/// <summary> Number nested/child nodes </summary>
			size_t NestedNodeCount()const;

			/// <summary>
			/// Nested/child node by index
			/// </summary>
			/// <param name="index"> Child node index </param>
			/// <returns> Nested node </returns>
			const Node& NestedNode(size_t index)const;

			/// <summary>
			/// Searches for a child node by name
			/// </summary>
			/// <param name="childName"> Name to search with </param>
			/// <param name="expectedIndex"> Index, the child node is expected to have </param>
			/// <returns> Child node if found, nullptr otherwise </returns>
			const Node* FindChildNodeByName(const std::string_view& childName, size_t expectedIndex = 0)const;


		private:
			// Owner
			const FBXContent* m_content = nullptr;
			
			// Index of the first character of the name inside m_stringBuffer
			size_t m_nameStart = 0;

			// Size of the name
			size_t m_nameLength = 0;

			// Index of the first property inside m_properties buffer
			size_t m_firstPropertyId = 0;

			// Number of node properties
			size_t m_propertyCount = 0;

			// Index of the first nested node inside m_nodes buffer
			size_t m_firstNestedNodeId = 0;

			// Number nested/child nodes
			size_t m_nestedNodeCount = 0;

			friend class FBXContent;
		};


		/// <summary>
		/// Extracts serialized node tree from a memory block, containing content form an FBX file 
		/// </summary>
		/// <param name="block"> Memory block (could be something like a memory-mapped FBX file) </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <returns> FBXContent if successful, nullptr otherwise </returns>
		static Reference<FBXContent> Decode(const MemoryBlock block, OS::Logger* logger);

		/// <summary>
		/// Extracts serialized node tree from a memory block, containing content form an FBX file 
		/// </summary>
		/// <param name="sourcePath"> FBX file path </param>
		/// <param name="logger"> Logger for error/warning reporting </param>
		/// <returns> FBXContent if successful, nullptr otherwise </returns>
		static Reference<FBXContent> Decode(const OS::Path& sourcePath, OS::Logger* logger);

		/// <summary> Default constructor </summary>
		inline FBXContent() = default;

		/// <summary> FBX Version </summary>
		uint32_t Version()const;

		/// <summary> Root node of the FBX tree </summary>
		const Node& RootNode()const;


	private:
		// FBX Version
		uint32_t m_version = 0;
		
		// Buffer for Node names and STRING Properties
		std::vector<char> m_stringBuffer;

		// Buffer for INT_16 Properties
		std::vector<int16_t> m_int16Buffer;

		// Buffer for INT_32 and INT_32_ARR Properties
		std::vector<int32_t> m_int32Buffer;

		// Buffer for INT_64 and INT_64_ARR Properties
		std::vector<int64_t> m_int64Buffer;

		// Buffer for FLOAT_32 and FLOAT_32_ARR Properties
		std::vector<float> m_float32Buffer;

		// Buffer for FLOAT_64 and FLOAT_64_ARR Properties
		std::vector<double> m_float64Buffer;

		// Buffer for boolean and RAW_BINARY Properties
		std::vector<uint8_t> m_rawBuffer;

		// FBX tree nodes (size is always positive and the first element represents the toot node)
		std::vector<Node> m_nodes;

		// Node property buffer (individual allocations per node would be costly for the performance, so we have one here)
		std::vector<Property> m_properties;

		// Copy/Move Construction/Assignment is blocked to prevent data corruption
		inline FBXContent(const FBXContent&) = delete;
		inline FBXContent& operator=(const FBXContent&) = delete;
	};


	/// <summary>
	/// Outputs FBXContent to a stream
	/// </summary>
	/// <param name="stream"> Output stream </param>
	/// <param name="content"> FBXContent to output </param>
	/// <returns> stream </returns>
	std::ostream& operator<<(std::ostream& stream, const FBXContent& content);

	/// <summary>
	/// Outputs FBXContent::Node alongside it's subnodes to a stream
	/// </summary>
	/// <param name="stream"> Output stream </param>
	/// <param name="content"> FBXContent::Node to output </param>
	/// <returns> stream </returns>
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Node& node);

	/// <summary>
	/// Outputs FBXContent::Property to a stream
	/// </summary>
	/// <param name="stream"> Output stream </param>
	/// <param name="content"> FBXContent::Property to output </param>
	/// <returns> stream </returns>
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Property& prop);
}
