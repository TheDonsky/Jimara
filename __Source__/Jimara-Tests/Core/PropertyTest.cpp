#include "../GtestHeaders.h"
#include "Core/Property.h"


namespace Jimara {
	namespace {
		struct Value {
			int value;

			inline Value(int val = 0) : value(val) {}

			inline static int GetValuePtr(const Value* self) { return self->value; }

			int GetValue()const { return value; }

			inline static void SetValuePtr(Value* self, const int& val) { self->value = val; }

			void SetValue(const int& val) { value = val; }

			void SetValueConst(const int&)const { }
		};
	}

	// Basic test for Property class
	TEST(PropertyTest, Basics) {
		{
			static int v;
			v = 1;
			Property<int> prop(v);
			EXPECT_EQ(prop, v);
			prop = 2;
			EXPECT_EQ(prop, v);
			EXPECT_EQ(v, 2);
		}
		{
			static const int v = 1;
			Property<int> prop(&v);
			EXPECT_EQ(prop, v);
			prop = 2;
			EXPECT_EQ(prop, v);
			EXPECT_EQ(v, 1);
		}
		{
			static int v;
			v = 1;
			Property<int> prop([]() { return v * 2; }, [](const int& value) { v = value; });
			EXPECT_EQ(prop, 2);
			prop = 2;
			EXPECT_EQ(prop, 4);
			EXPECT_EQ(v, 2);
		}
		{
			Value v(1);
			Function<int> get(Value::GetValuePtr, &v);
			Callback<const int&> set(Value::SetValuePtr, &v);
			Property<int> prop(get, set);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
		{
			Value v(1);
			Property<int> prop(Value::GetValuePtr, Value::SetValuePtr, v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
		{
			Value v(1);
			Property<int> prop(Value::GetValuePtr, Value::SetValuePtr, &v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
		{
			Value v(1);
			Property<int> prop(&Value::GetValue, &Value::SetValue, v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
		{
			Value v(1);
			Property<int> prop(&Value::GetValue, &Value::SetValue, &v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
		{
			const Value v(1);
			Property<int> prop(&Value::GetValue, &Value::SetValueConst, v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 1);
		}
		{
			Value v(1);
			Property<int> prop(&Value::GetValue, &Value::SetValueConst, &v);
			EXPECT_EQ(prop, v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 1);
		}
	}
}
