#pragma once
#include "../../../Core/Memory/MemoryBlock.h"
#include "../../../OS/Logging/Logger.h"
#include <ostream>


namespace Jimara {
	class FBXContent : public virtual Object {
	public:
		enum class PropertyType : uint8_t {
			UNKNOWN = 0,
			BOOLEAN = 1,
			BOOLEAN_ARR = 2,
			INT_16 = 3,
			INT_32 = 4,
			INT_32_ARR = 5,
			INT_64 = 6,
			INT_64_ARR = 7,
			FLOAT_32 = 8,
			FLOAT_32_ARR = 9,
			FLOAT_64 = 10,
			FLOAT_64_ARR = 11,
			STRING = 12,
			RAW_BINARY = 13,
			PROPERTY_TYPE_COUNT = 14
		};

		class Property {
		public:
			PropertyType Type()const;

			size_t Count()const;

			operator bool()const;

			bool BoolElem(size_t index)const;

			operator int16_t()const;

			operator int32_t()const;

			int32_t Int32Elem(size_t index)const;

			operator int64_t()const;

			int64_t Int64Elem(size_t index)const;

			operator float()const;

			float Float32Elem(size_t index)const;

			operator double()const;

			double Float64Elem(size_t index)const;

			operator const std::string_view()const;

			operator MemoryBlock()const;

		private:
			const FBXContent* m_content = nullptr;
			PropertyType m_type = PropertyType::UNKNOWN;
			size_t m_valueOffset = 0;
			size_t m_valueCount = 0;

			friend class FBXContent;
		};

		class Node {
		public:
			const std::string_view Name()const;

			size_t PropertyCount()const;

			const Property& NodeProperty(size_t index)const;

			size_t NestedNodeCount()const;

			const Node& NestedNode(size_t index)const;

		private:
			const FBXContent* m_content = nullptr;
			size_t m_nameStart = 0;
			size_t m_firstPropertyId = 0;
			size_t m_propertyCount = 0;
			size_t m_firstNestedNodeId = 0;
			size_t m_nestedNodeCount = 0;

			friend class FBXContent;
		};


		static Reference<FBXContent> Decode(const MemoryBlock block, OS::Logger* logger);

		inline FBXContent() = default;

		uint32_t Version()const;

		const Node& RootNode()const;

	private:
		uint32_t m_version = 0;
		
		std::vector<char> m_stringBuffer;
		std::vector<int16_t> m_int16Buffer;
		std::vector<int32_t> m_int32Buffer;
		std::vector<int64_t> m_int64Buffer;
		std::vector<float> m_float32Buffer;
		std::vector<double> m_float64Buffer;
		std::vector<uint8_t> m_rawBuffer;

		std::vector<Node> m_nodes;
		std::vector<Property> m_properties;

		inline FBXContent(const FBXContent&) = delete;
		inline FBXContent& operator=(const FBXContent&) = delete;
	};

	std::ostream& operator<<(std::ostream& stream, const FBXContent& content);
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Node& node);
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Property& prop);
}
