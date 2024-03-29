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
			static int v;
			v = 1;
			Property<const int> prop(v);
			EXPECT_EQ(prop, v);
			prop = 2;
			EXPECT_EQ(prop, v);
			EXPECT_EQ(v, 1);
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
		{
			int v(1);
			Property<const int&> prop(v);
			EXPECT_EQ(prop, v);
			const int& valRef = prop;
			EXPECT_EQ(&valRef, &v);
			prop = 2;
			EXPECT_EQ(prop, v);
			EXPECT_EQ(v, 1);
		}
		{
			Value v(1);
			auto get = [&]() -> const int& { return v.value; };
			auto set = [&](const int& val) { return v.SetValue(val); };
			auto getFn = Function<const int&>::FromCall(&get);
			auto setFn = Callback<const int&>::FromCall(&set);
			Property<int&> prop(getFn, setFn);
			EXPECT_EQ(prop, v.value);
			const int& valRef = prop;
			EXPECT_EQ(&valRef, &v.value);
			prop = 2;
			EXPECT_EQ(prop, v.value);
			EXPECT_EQ(v.value, 2);
		}
	}


	// Test for Property to property assignment
	TEST(PropertyTest, PropertyToPropertyAssignment) {
		{
			static int v;
			v = 1;
			Property<int> propA(v);
			Property<int> propB(v);
			EXPECT_EQ(propA, v);
			EXPECT_EQ(propB, v);
			propB = 2;
			EXPECT_EQ(propA, v);
			EXPECT_EQ(propB, v);
			EXPECT_EQ(v, 2);
		}
		{
			int v1 = 1, v2 = 2;
			Property<int> propA(v1);
			Property<int> propB(v2);
			EXPECT_EQ(propA, v1);
			EXPECT_EQ(propB, v2);
			propA = propB;
			propB = 3;
			EXPECT_EQ(propA, v1);
			EXPECT_EQ(propB, v2);
			EXPECT_EQ(v1, 2);
			EXPECT_EQ(v2, 3);
		}
		{
			int v1 = 1, v2 = 2;
			[&]() { return Property<int>(v1); }() = Property<int>(v2);
			EXPECT_EQ(v1, 2);
			EXPECT_EQ(v2, 2);
		}
	}
}
