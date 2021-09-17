#include "FBXContent.h"
#include <cstring>

namespace Jimara {
	// Property:
	FBXContent::PropertyType FBXContent::Property::Type()const { return m_type; }

	size_t FBXContent::Property::Count()const { return m_valueCount; }

	FBXContent::Property::operator bool()const { return BoolElem(0); }

	bool FBXContent::Property::BoolElem(size_t index)const { return static_cast<bool>(m_content->m_rawBuffer[m_valueOffset + index]); }

	FBXContent::Property::operator int16_t()const { return m_content->m_int16Buffer[m_valueOffset]; }

	FBXContent::Property::operator int32_t()const { return Int32Elem(0); }

	int32_t FBXContent::Property::Int32Elem(size_t index)const { return m_content->m_int32Buffer[m_valueOffset + index]; }

	FBXContent::Property::operator int64_t()const { return Int64Elem(0); }

	int64_t FBXContent::Property::Int64Elem(size_t index)const { return m_content->m_int64Buffer[m_valueOffset + index]; }

	FBXContent::Property::operator float()const { return Float32Elem(0); }

	float FBXContent::Property::Float32Elem(size_t index)const { return m_content->m_float32Buffer[m_valueOffset + index]; }

	FBXContent::Property::operator double()const { return Float64Elem(0); }

	double FBXContent::Property::Float64Elem(size_t index)const { return m_content->m_float64Buffer[m_valueOffset + index]; }

	FBXContent::Property::operator const std::string_view()const { return m_content->m_stringBuffer.data() + m_valueOffset; }

	FBXContent::Property::operator MemoryBlock()const {
		typedef MemoryBlock(*TypeCastFn)(const FBXContent::Property* self);
		static const size_t PROPERTY_TYPE_COUNT = static_cast<size_t>(PropertyType::PROPERTY_TYPE_COUNT);
		static const TypeCastFn* const CAST_FUNCTIONS = []() -> const TypeCastFn* {
			static TypeCastFn functions[PROPERTY_TYPE_COUNT];

			const TypeCastFn unknown = [](const FBXContent::Property* self) { return MemoryBlock(nullptr, 0, self->m_content); };
			for (size_t i = 0; i < PROPERTY_TYPE_COUNT; i++) functions[i] = unknown;

			functions[static_cast<size_t>(PropertyType::RAW_BINARY)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_rawBuffer.data() + self->m_valueOffset, self->m_valueCount, self->m_content); };
			functions[static_cast<size_t>(PropertyType::BOOLEAN)] = functions[static_cast<size_t>(PropertyType::BOOLEAN_ARR)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_rawBuffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(bool), self->m_content); };
			functions[static_cast<size_t>(PropertyType::INT_16)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_int16Buffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(int16_t), self->m_content); };
			functions[static_cast<size_t>(PropertyType::INT_32)] = functions[static_cast<size_t>(PropertyType::INT_32_ARR)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_int32Buffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(int32_t), self->m_content); };
			functions[static_cast<size_t>(PropertyType::INT_64)] = functions[static_cast<size_t>(PropertyType::INT_64_ARR)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_int64Buffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(int64_t), self->m_content); };
			functions[static_cast<size_t>(PropertyType::FLOAT_32)] = functions[static_cast<size_t>(PropertyType::FLOAT_32_ARR)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_float32Buffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(float), self->m_content); };
			functions[static_cast<size_t>(PropertyType::FLOAT_64)] = functions[static_cast<size_t>(PropertyType::FLOAT_64_ARR)] =
				[](const FBXContent::Property* self) { return MemoryBlock(self->m_content->m_float64Buffer.data() + self->m_valueOffset, self->m_valueCount * sizeof(double), self->m_content); };
			functions[static_cast<size_t>(PropertyType::STRING)] =
				[](const FBXContent::Property* self) { return MemoryBlock(((const std::string_view)*self).data(), self->m_valueCount * sizeof(char), self->m_content); };

			return functions;
		}();

		return (m_type < PropertyType::PROPERTY_TYPE_COUNT) ? CAST_FUNCTIONS[static_cast<size_t>(m_type)](this) : MemoryBlock(nullptr, 0, m_content);
	}





	// Node:
	const std::string_view FBXContent::Node::Name()const { return m_content->m_stringBuffer.data() + m_nameStart; }

	size_t FBXContent::Node::PropertyCount()const { return m_propertyCount; }

	const FBXContent::Property& FBXContent::Node::NodeProperty(size_t index)const { return m_content->m_properties[m_firstPropertyId + index]; }

	size_t FBXContent::Node::NestedNodeCount()const { return m_nestedNodeCount; }

	const FBXContent::Node& FBXContent::Node::NestedNode(size_t index)const { return m_content->m_nodes[m_firstNestedNodeId + index]; }





	// Decode:
	namespace {
		static const char FBX_BINARY_HEADER[] = "Kaydara FBX Binary  ";
		static constexpr inline size_t FbxBinaryHeaderSize() { return sizeof(FBX_BINARY_HEADER) / sizeof(char); }

		static const uint8_t FBX_BINARY_HEADER_MAGIC[] = { 0x1A, 0x00 };
		static constexpr inline size_t FbxBinaryHeaderMagicSize() { return sizeof(FBX_BINARY_HEADER_MAGIC) / sizeof(char); }

		const size_t NULL_RECORD_SIZE = 13;

		static const uint8_t PropertyTypeCode_BOOLEAN = static_cast<uint8_t>('C');
		static const uint8_t PropertyTypeCode_BOOLEAN_ARR = static_cast<uint8_t>('b');
		static const uint8_t PropertyTypeCode_INT_16 = static_cast<uint8_t>('Y');
		static const uint8_t PropertyTypeCode_INT_32 = static_cast<uint8_t>('I');
		static const uint8_t PropertyTypeCode_INT_32_ARR = static_cast<uint8_t>('i');
		static const uint8_t PropertyTypeCode_INT_64 = static_cast<uint8_t>('L');
		static const uint8_t PropertyTypeCode_INT_64_ARR = static_cast<uint8_t>('l');
		static const uint8_t PropertyTypeCode_FLOAT_32 = static_cast<uint8_t>('F');
		static const uint8_t PropertyTypeCode_FLOAT_32_ARR = static_cast<uint8_t>('f');
		static const uint8_t PropertyTypeCode_FLOAT_64 = static_cast<uint8_t>('D');
		static const uint8_t PropertyTypeCode_FLOAT_64_ARR = static_cast<uint8_t>('d');
		static const uint8_t PropertyTypeCode_STRING = static_cast<uint8_t>('S');
		static const uint8_t PropertyTypeCode_RAW_BINARY = static_cast<uint8_t>('R');

		static const Endian FBX_BINARY_ENDIAN = Endian::LITTLE;
	}

	Reference<FBXContent> FBXContent::Decode(const MemoryBlock block, OS::Logger* logger) {
		static auto logError = [](OS::Logger* logger, auto... msg) -> bool {
			if (logger != nullptr) logger->Error(msg...);
			return false;
		};

		auto error = [&](auto... msg) -> bool { return logError(logger, msg...); };

		auto warning = [&](auto... msg) { if (logger != nullptr) logger->Warning(msg...); };

		const Reference<FBXContent> content = Object::Instantiate<FBXContent>();

		auto parseBinary = [&]() -> bool {
			// Header is already parsed if we reached this point:
			size_t ptr = FbxBinaryHeaderSize();
			
			// Checks if the buffer will overflow with given amount of bytes after ptr:
			auto bufferOverflow = [&](size_t bytesAfterPtr) { return (block.Size() < (ptr + bytesAfterPtr)); };

			// Header magic:
			{
				if (bufferOverflow(FbxBinaryHeaderMagicSize()))
					return error("FBXContent::Decode::parseBinary - Memory block does not include header magic bytes!");
				else for (size_t i = 0; i < FbxBinaryHeaderMagicSize(); i++) {
					const uint32_t got = (uint32_t)block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN);
					const uint32_t expected = (uint32_t)FBX_BINARY_HEADER_MAGIC[i];
					if (got != expected)
						warning("FBXContent::Decode::parseBinary - Header magic byte mismatch at index ", i, " (got: ", got, "; expected: ", expected, ")");
				}
			}

			// Version:
			{
				if (bufferOverflow(sizeof(uint32_t)))
					return error("FBXContent::Decode::parseBinary - Memory block does not include FBX version number!");
				else {
					content->m_version = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
					// __TODO__: Maybe... Warn about versions that are not yet supported?
				}
			}

			// Property record reader:
			auto parsePropertyRecord = [&]() -> bool {
				// Type code:
				if (bufferOverflow(sizeof(char)))
					return error("FBXContent::Decode::parseBinary::parsePropertyRecord - Buffer overflow at TypeCode!");
				const char typeCode = block.Get<char>(ptr, FBX_BINARY_ENDIAN);

				// Parse functions:
				typedef bool(*ParsePropertyValueFn)(char, Property&, FBXContent*, const MemoryBlock&, size_t&, OS::Logger*);
				static const ParsePropertyValueFn* const PARSE_FUNCTIONS = []() -> const ParsePropertyValueFn* {
					static const size_t MAX_CHAR = sizeof(char) << 8;
					static ParsePropertyValueFn functions[MAX_CHAR];

					// Unknown type code:
					const ParsePropertyValueFn unknownTypeCode = [](char key, Property&, FBXContent*, const MemoryBlock&, size_t&, OS::Logger* logger) -> bool {
						return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey not recognized <", static_cast<uint32_t>(key), "/'", key, "'>!");
					};
					for (size_t i = 0; i < MAX_CHAR; i++) functions[i] = unknownTypeCode;

					// Single value read:
					static const auto readSingle = [](
						char key, Property& prop, const MemoryBlock& block, size_t& ptr, OS::Logger* logger, 
						size_t unitSize, PropertyType propertyType, size_t valueOffset, auto pushFn) -> bool {
							if ((ptr + unitSize) > block.Size())
								return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow!");
							prop.m_type = propertyType;
							prop.m_valueOffset = valueOffset;
							prop.m_valueCount = 1;
							pushFn();
							return true;
					};

					// Multi value read:
					static const auto readArray = [](char key, Property& prop, const MemoryBlock& block, size_t& ptr, OS::Logger* logger,
						size_t unitSize, PropertyType propertyType, size_t valueOffset, auto pushFn) -> bool {
							if ((ptr + (sizeof(uint32_t) * 3)) > block.Size())
								return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on array header!");
							prop.m_type = propertyType;
							prop.m_valueOffset = valueOffset;
							prop.m_valueCount = static_cast<size_t>(block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN));
							const uint32_t encoding = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
							const uint32_t compressedLength = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
							MemoryBlock dataBlock(nullptr, 0, nullptr);
							if (encoding == 0) {
								const size_t arrayByteCount = unitSize * prop.m_valueCount;
								if ((ptr + arrayByteCount) > block.Size())
									return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on array data!");
								dataBlock = MemoryBlock(static_cast<const char*>(block.Data()) + ptr, arrayByteCount, nullptr);
								ptr += arrayByteCount;
							}
							else if (encoding == 1) {
								// __TODO__: Implement zipped case properly...
								logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Zip compressed array encoding<", encoding, "> not yet supported!");
								prop.m_type = PropertyType::RAW_BINARY;
								prop.m_valueOffset = prop.m_content->m_rawBuffer.size();
								prop.m_valueCount = compressedLength;
								if ((ptr + compressedLength) > block.Size())
									logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow with zip-compressed data!");
								for (size_t i = 0; i < compressedLength; i++)
									((FBXContent*)(void*)prop.m_content)->m_rawBuffer.push_back(block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN));
							}
							else return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Unsupported array encoding<", encoding, ">!");
							if (encoding == 0) // __TODO__: Remove this, when you have working zlib implementation...
							pushFn(dataBlock, 0, prop.m_valueCount);
							return true;
					};

					// Boolean:
					functions[PropertyTypeCode_BOOLEAN] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(uint8_t), PropertyType::BOOLEAN, content->m_rawBuffer.size(),
							[&]() { content->m_rawBuffer.push_back(static_cast<bool>(block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN))); });
					};

					// Boolean array:
					functions[PropertyTypeCode_BOOLEAN_ARR] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readArray(key, prop, block, ptr, logger, sizeof(uint8_t), PropertyType::BOOLEAN_ARR, content->m_rawBuffer.size(),
							[&](MemoryBlock dataBlock, size_t dataPtr, size_t count) {
								for (size_t i = 0; i < count; i++)
									content->m_rawBuffer.push_back(static_cast<bool>(dataBlock.Get<uint8_t>(dataPtr, FBX_BINARY_ENDIAN)));
							});
					};

					// 16 bit integer:
					functions[PropertyTypeCode_INT_16] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(int16_t), PropertyType::INT_16, content->m_int16Buffer.size(),
							[&]() { content->m_int16Buffer.push_back(block.Get<int16_t>(ptr, FBX_BINARY_ENDIAN)); });
					};

					// 32 bit integer:
					functions[PropertyTypeCode_INT_32] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(int32_t), PropertyType::INT_32, content->m_int32Buffer.size(),
							[&]() { content->m_int32Buffer.push_back(block.Get<int32_t>(ptr, FBX_BINARY_ENDIAN)); });
					};

					// 32 bit integer array:
					functions[PropertyTypeCode_INT_32_ARR] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readArray(key, prop, block, ptr, logger, sizeof(int32_t), PropertyType::INT_32_ARR, content->m_int32Buffer.size(),
							[&](MemoryBlock dataBlock, size_t dataPtr, size_t count) {
								for (size_t i = 0; i < count; i++)
									content->m_int32Buffer.push_back(dataBlock.Get<int32_t>(dataPtr, FBX_BINARY_ENDIAN));
							});
					};

					// 64 bit integer:
					functions[PropertyTypeCode_INT_64] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(int64_t), PropertyType::INT_64, content->m_int64Buffer.size(),
							[&]() { content->m_int64Buffer.push_back(block.Get<int64_t>(ptr, FBX_BINARY_ENDIAN)); });
					};

					// 64 bit integer array:
					functions[PropertyTypeCode_INT_64_ARR] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readArray(key, prop, block, ptr, logger, sizeof(int64_t), PropertyType::INT_64_ARR, content->m_int64Buffer.size(),
							[&](MemoryBlock dataBlock, size_t dataPtr, size_t count) {
								for (size_t i = 0; i < count; i++)
									content->m_int64Buffer.push_back(dataBlock.Get<int64_t>(dataPtr, FBX_BINARY_ENDIAN));
							});
					};

					// 32 bit floating point:
					functions[PropertyTypeCode_FLOAT_32] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(float), PropertyType::FLOAT_32, content->m_float32Buffer.size(),
							[&]() { content->m_float32Buffer.push_back(block.Get<float>(ptr, FBX_BINARY_ENDIAN)); });
					};

					// 32 bit floating point array:
					functions[PropertyTypeCode_FLOAT_32_ARR] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readArray(key, prop, block, ptr, logger, sizeof(float), PropertyType::FLOAT_32_ARR, content->m_float32Buffer.size(),
							[&](MemoryBlock dataBlock, size_t dataPtr, size_t count) {
								for (size_t i = 0; i < count; i++)
									content->m_float32Buffer.push_back(dataBlock.Get<float>(dataPtr, FBX_BINARY_ENDIAN));
							});
					};

					// 64 bit floating point:
					functions[PropertyTypeCode_FLOAT_64] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readSingle(key, prop, block, ptr, logger, sizeof(double), PropertyType::FLOAT_64, content->m_float64Buffer.size(),
							[&]() { content->m_float64Buffer.push_back(block.Get<double>(ptr, FBX_BINARY_ENDIAN)); });
					};

					// 64 bit floating point array:
					functions[PropertyTypeCode_FLOAT_64_ARR] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						return readArray(key, prop, block, ptr, logger, sizeof(double), PropertyType::FLOAT_64_ARR, content->m_float64Buffer.size(),
							[&](MemoryBlock dataBlock, size_t dataPtr, size_t count) {
								for (size_t i = 0; i < count; i++)
									content->m_float64Buffer.push_back(dataBlock.Get<double>(dataPtr, FBX_BINARY_ENDIAN));
							});
					};

					// String:
					functions[PropertyTypeCode_STRING] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						if ((ptr + sizeof(uint32_t)) > block.Size())
							return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on string Length!");
						prop.m_type = PropertyType::STRING;
						prop.m_valueOffset = content->m_stringBuffer.size();
						prop.m_valueCount = static_cast<size_t>(block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN));
						if ((ptr + prop.m_valueCount) > block.Size())
							return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on string Data!");
						for (size_t i = 0; i < prop.m_valueCount; i++)
							content->m_stringBuffer.push_back(block.Get<char>(ptr, FBX_BINARY_ENDIAN));
						content->m_stringBuffer.push_back('\0');
						return true;
					};

					// Raw data:
					functions[PropertyTypeCode_RAW_BINARY] = [](char key, Property& prop, FBXContent* content, const MemoryBlock& block, size_t& ptr, OS::Logger* logger) -> bool {
						if ((ptr + sizeof(uint32_t)) > block.Size())
							return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on raw data Length!");
						prop.m_type = PropertyType::RAW_BINARY;
						prop.m_valueOffset = content->m_rawBuffer.size();
						prop.m_valueCount = static_cast<size_t>(block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN));
						if ((ptr + prop.m_valueCount) > block.Size())
							return logError(logger, "FBXContent::Decode::parseBinary::parsePropertyRecord - TypeKey['", key, "']: Buffer overflow on raw binary Data!");
						for (size_t i = 0; i < prop.m_valueCount; i++)
							content->m_rawBuffer.push_back(block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN));
						return true;
					};

					return functions;
				}();
				Property prop;
				prop.m_content = content;
				if (PARSE_FUNCTIONS[static_cast<uint8_t>(typeCode)](typeCode, prop, content, block, ptr, logger)) {
					content->m_properties.push_back(prop);
					return true;
				}
				else return false;
			};

			// Counts number of nested nodes (only if they exist)
			auto countChildNodes = [&](size_t parentEnd, size_t& nodeCount) -> bool {
				if (parentEnd < NULL_RECORD_SIZE)
					return error("FBXContent::Decode::parseBinary::countChildNodes - End offset less than ", NULL_RECORD_SIZE, "!");
				const size_t endByte = (parentEnd - NULL_RECORD_SIZE);
				if (ptr > endByte)
					return error("FBXContent::Decode::parseBinary::countChildNodes - Properties and nested records overlap!");
				size_t nodePtr = ptr;
				nodeCount = 0;
				while (nodePtr < endByte) {
					if ((nodePtr + sizeof(uint32_t)) > block.Size())
						return error("FBXContent::Decode::parseBinary::countChildNodes - Buffer overflow when reading EndOffset!");
					uint32_t endOffset = block.Get<uint32_t>(nodePtr, FBX_BINARY_ENDIAN);
					if (endOffset > endByte || nodePtr >= endOffset)
						return error("FBXContent::Decode::parseBinary::countChildNodes - Invalid EndOffset!");
					nodePtr = endOffset;
					nodeCount++;
				}
				if (nodePtr != endByte)
					return error("FBXContent::Decode::parseBinary::countChildNodes - Nested record overlaps with NULL-record!");
				for (size_t i = 0; i < NULL_RECORD_SIZE; i++)
					if (block.Get<uint8_t>(nodePtr, FBX_BINARY_ENDIAN) != 0)
						warning("FBXContent::Decode::parseBinary::countChildNodes - NULL-record not filled with zeroes!");
				return true;
			};

			size_t childNodeStartId = 0;

			// Node record reader:
			auto parseNodeRecord = [&](size_t nodeId, auto parseSubNode) {
				Node node;
				node.m_content = content;

				if (bufferOverflow(sizeof(uint32_t) * 3 + sizeof(uint8_t)))
					return error("FBXContent::Decode::parseBinary::parseNodeRecord - Memory block too small to read node record's header!");
				
				// End offset:
				const uint32_t endOffset = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
				if (block.Size() < endOffset)
					return error("FBXContent::Decode::parseBinary::parseNodeRecord - EndOffset implies buffer overflow!");

				// Property count and buffer chunk size:
				const uint32_t numProperties = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
				const uint32_t propertyListLen = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);
				node.m_firstPropertyId = content->m_properties.size();
				node.m_propertyCount = static_cast<size_t>(numProperties);

				// Name:
				const uint8_t nameLen = block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN);
				if (bufferOverflow((size_t)nameLen))
					return error("FBXContent::Decode::parseBinary::parseNodeRecord - NameLen implies buffer overflow!");
				node.m_nameStart = content->m_stringBuffer.size();
				for (uint8_t i = 0; i < nameLen; i++)
					content->m_stringBuffer.push_back(block.Get<char>(ptr, FBX_BINARY_ENDIAN));
				content->m_stringBuffer.push_back('\0');

				// Property list:
				const size_t nestedListPtr = ptr + ((uint32_t)propertyListLen);
				if (nestedListPtr > endOffset)
					return error("FBXContent::Decode::parseBinary::parseNodeRecord - PropertyListLen implies buffer overflow!");
				for (uint32_t i = 0; i < numProperties; i++)
					if (!parsePropertyRecord()) return false;
				if (ptr != nestedListPtr)
					return error("FBXContent::Decode::parseBinary::parseNodeRecord - PropertyListLen does not match what's observed!");

				// Read nested records:
				if (ptr < endOffset) {
					node.m_firstNestedNodeId = childNodeStartId;
					if (!countChildNodes(endOffset, node.m_nestedNodeCount)) return false;
					childNodeStartId += node.m_nestedNodeCount;
					const size_t endByte = (endOffset - NULL_RECORD_SIZE);
					size_t index = 0;
					while (ptr < endByte) {
						if (!parseSubNode(node.m_firstNestedNodeId + index, parseSubNode)) return false;
						index++;
					}
					if (ptr != endByte)
						return error("FBXContent::Decode::parseBinary::parseNodeRecord - Nested record overlaps with NULL-record!");
					for (size_t i = 0; i < NULL_RECORD_SIZE; i++)
						if (block.Get<uint8_t>(ptr, FBX_BINARY_ENDIAN) != 0)
							warning("FBXContent::Decode::parseBinary::parseNodeRecord - NULL-record not filled with zeroes!");
				}
				
				while (content->m_nodes.size() <= nodeId)
					content->m_nodes.push_back(Node());
				content->m_nodes[nodeId] = node;
				return true;
			};

			while (ptr < block.Size()) {
				if (bufferOverflow(NULL_RECORD_SIZE))
					return error("FBXContent::Decode::parseBinary - Reading NULL-record will cause a buffer overflow!");
				size_t nullCheckPtr = ptr;
				bool isNullRecord = true;
				for (size_t i = 0; i < NULL_RECORD_SIZE; i++)
					if (block.Get<uint8_t>(nullCheckPtr, FBX_BINARY_ENDIAN) != 0) {
						isNullRecord = false;
						break;
					}
				if (isNullRecord) break;
				else {
					content->m_rootNodes.push_back(content->m_nodes.size());
					childNodeStartId = content->m_rootNodes.back() + 1;
					if (!parseNodeRecord(content->m_rootNodes.back(), parseNodeRecord)) return false;
				}
			}
			return true;
		};

		if (block.Size() >= FbxBinaryHeaderSize() && (memcmp(FBX_BINARY_HEADER, block.Data(), FbxBinaryHeaderSize())) == 0) {
			if (parseBinary()) return content;
			else return nullptr;
		}
		else {
			error("FBXContent::Decode - Memory block is invalid or of an unsupported format!");
			return nullptr;
		}
	}





	// FBXContent:
	uint32_t FBXContent::Version()const { return m_version; }

	size_t FBXContent::RootNodeCount()const { return m_rootNodes.size(); }

	const FBXContent::Node& FBXContent::RootNode(size_t index)const { return m_nodes[m_rootNodes[index]]; }


	// Stream:
	namespace {
		static thread_local std::vector<std::string> STREAM_INSETS = { "" };
		static thread_local size_t STREAM_INSET_ID = 0;
		inline static void PushStreamInset() {
			STREAM_INSET_ID++;
			while (STREAM_INSETS.size() <= STREAM_INSET_ID)
				STREAM_INSETS.push_back(STREAM_INSETS.back() + "  ");
		}
		inline static void PopStreamInset() {
			STREAM_INSET_ID--;
		}
		inline static const std::string& StreamInset() {
			return STREAM_INSETS[STREAM_INSET_ID];
		}
	}

	std::ostream& operator<<(std::ostream& stream, const FBXContent& content) {
		PushStreamInset();
		stream << "FBXContent at " << ((const void*)(&content)) << ": {" << std::endl
			<< StreamInset() << "Version: " << content.Version() << "; " << std::endl;
		for (size_t i = 0; i < content.RootNodeCount(); i++)
			stream << content.RootNode(i);
		stream << '}' << std::endl;
		PopStreamInset();
		return stream;
	}
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Node& node) {
		stream << StreamInset() << "[" << node.Name() << "]: ";
		for (size_t i = 0; i < node.PropertyCount(); i++) {
			if (i > 0) stream << ", ";
			stream << node.NodeProperty(i);
		}
		if (node.PropertyCount() > 0) stream << ' ';
		stream << '{';
		if (node.NestedNodeCount() > 0) {
			stream << std::endl;
			PushStreamInset();
			for (size_t i = 0; i < node.NestedNodeCount(); i++)
				stream << node.NestedNode(i);
			PopStreamInset();
			stream << StreamInset() << "}" << std::endl;
		}
		else stream << "}" << std::endl;
		return stream;
	}
	std::ostream& operator<<(std::ostream& stream, const FBXContent::Property& prop) {
		typedef void(*PrintFn)(std::ostream& stream, const FBXContent::Property& prop);
		static const PrintFn UNKNOWN = [](std::ostream& stream, const FBXContent::Property& prop) { stream << "<ERR_TYPE" << (size_t)prop.Type() << ">"; };
		static const PrintFn* const PRINT_FUNCTIONS = []() -> const PrintFn* {
			static PrintFn functions[static_cast<size_t>(FBXContent::PropertyType::PROPERTY_TYPE_COUNT)];
			for (size_t i = 0; i < static_cast<size_t>(FBXContent::PropertyType::PROPERTY_TYPE_COUNT); i++)
				functions[i] = UNKNOWN;

			static const auto printOne = [](std::ostream& stream, char symbol, auto getValue) {
				stream << "<'" << symbol << "'" << getValue() << '>';
			};

			static const auto printMany = [](std::ostream& stream, char symbol, size_t count, auto getValue) {
				stream << "<'" << symbol << "'(";
				for (size_t i = 0; i < count; i++) {
					if (i > 0) stream << "; ";
					stream << getValue(i);
				}
				stream << ")>";
			};

			functions[static_cast<size_t>(FBXContent::PropertyType::BOOLEAN)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_BOOLEAN, [&]() { return (bool)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::BOOLEAN_ARR)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printMany(stream, PropertyTypeCode_BOOLEAN_ARR, prop.Count(), [&](size_t i) { return prop.BoolElem(i); }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::INT_16)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_INT_16, [&]() { return (int16_t)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::INT_32)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_INT_32, [&]() { return (int32_t)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::INT_32_ARR)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printMany(stream, PropertyTypeCode_INT_32_ARR, prop.Count(), [&](size_t i) { return prop.Int32Elem(i); }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::INT_64)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_INT_64, [&]() { return (int64_t)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::INT_64_ARR)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printMany(stream, PropertyTypeCode_INT_64_ARR, prop.Count(), [&](size_t i) { return prop.Int64Elem(i); }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::FLOAT_32)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_FLOAT_32, [&]() { return (float)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::FLOAT_32_ARR)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printMany(stream, PropertyTypeCode_FLOAT_32_ARR, prop.Count(), [&](size_t i) { return prop.Float32Elem(i); }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::FLOAT_64)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_FLOAT_64, [&]() { return (double)prop; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::FLOAT_64_ARR)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printMany(stream, PropertyTypeCode_FLOAT_64_ARR, prop.Count(), [&](size_t i) { return prop.Float64Elem(i); }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::STRING)] =
				[](std::ostream& stream, const FBXContent::Property& prop) { printOne(stream, PropertyTypeCode_STRING, [&]() { return std::string("\"") + ((std::string_view)prop).data() + "\""; }); };
			functions[static_cast<size_t>(FBXContent::PropertyType::RAW_BINARY)] =
				[](std::ostream& stream, const FBXContent::Property&) { stream << "<RAW>"; };

			return functions;
		}();
		if (prop.Type() < FBXContent::PropertyType::PROPERTY_TYPE_COUNT)
			PRINT_FUNCTIONS[static_cast<size_t>(prop.Type())](stream, prop);
		else UNKNOWN(stream, prop);
		return stream;
	}
}
