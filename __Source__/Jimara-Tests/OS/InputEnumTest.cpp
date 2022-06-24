#include "../GtestHeaders.h"
#include <vector>
#include "OS/Input/Input.h"
#include "Data/Serialization/Attributes/EnumAttribute.h"

namespace Jimara {
	namespace OS {
		TEST(InputEnumTest, KeyCodeEnumAttribute) {
			const Object* object = Input::KeyCodeEnumAttribute();
			EXPECT_NE(object, nullptr);
			typedef std::underlying_type_t<Input::KeyCode> UnderlyingType;
			typedef Serialization::EnumAttribute<UnderlyingType> AttributeType;
			const AttributeType* attribute = dynamic_cast<const AttributeType*>(object);
			ASSERT_NE(attribute, nullptr);
			EXPECT_EQ((size_t)Input::KeyCode::NONE, 0u);
			EXPECT_EQ(attribute->ChoiceCount(), (size_t)Input::KeyCode::KEYCODE_COUNT);
			for (size_t i = 0; i < attribute->ChoiceCount(); i++)
				EXPECT_EQ(attribute->operator[](i).value, i);
		}

		TEST(InputEnumTest, AxisEnumAttribute) {
			const Object* object = Input::AxisEnumAttribute();
			EXPECT_NE(object, nullptr);
			typedef std::underlying_type_t<Input::Axis> UnderlyingType;
			typedef Serialization::EnumAttribute<UnderlyingType> AttributeType;
			const AttributeType* attribute = dynamic_cast<const AttributeType*>(object);
			ASSERT_NE(attribute, nullptr);
			EXPECT_EQ((size_t)Input::Axis::NONE, 0u);
			EXPECT_EQ(attribute->ChoiceCount(), (size_t)Input::Axis::AXIS_COUNT);
			for (size_t i = 0; i < attribute->ChoiceCount(); i++)
				EXPECT_EQ(attribute->operator[](i).value, i);
		}
	}
}
