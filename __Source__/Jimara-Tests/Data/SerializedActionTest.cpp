#include "../GtestHeaders.h"
#include "../Memory.h"
#include "../CountingLogger.h"
#include "Data/Serialization/SerializedAction.h"
#include <tuple>


namespace Jimara {
	namespace Serialization {
		// Basic tests for a callback with no arguments
		TEST(SerializedActionTest, NoArguments) {
			int callCount = 0;
			auto call = [&]() { callCount++; };
			const Callback<> callback = Callback<>::FromCall(&call);

			SerializedCallback action = SerializedCallback::Create<>::From("Call", callback);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(callCount, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 0u);
			ASSERT_EQ(callCount, 0);

			instance->Invoke();
			ASSERT_EQ(callCount, 1);

			instance->Invoke();
			ASSERT_EQ(callCount, 2);

			size_t fieldCount = 0u;
			auto examineField = [&](const SerializedObject& item) { fieldCount++; };
			instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			ASSERT_EQ(fieldCount, 0u);

			instance->Invoke();
			ASSERT_EQ(callCount, 3);
		}

		// Basic tests for a function return value with no arguments
		TEST(SerializedActionTest, NoArguments_ReturnValue) {
			int callCount = 0;
			auto call = [&]() -> int { callCount++; return callCount; };
			const Function<int> function = Function<int>::FromCall(&call);

			SerializedAction action = SerializedAction<int>::Create<>::From("Call", function);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(callCount, 0);

			Reference<SerializedAction<int>::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 0u);
			ASSERT_EQ(callCount, 0);

			ASSERT_EQ(instance->Invoke(), 1);
			ASSERT_EQ(callCount, 1);

			ASSERT_EQ(instance->Invoke(), 2);
			ASSERT_EQ(callCount, 2);

			size_t fieldCount = 0u;
			auto examineField = [&](const SerializedObject& item) { fieldCount++; };
			instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			ASSERT_EQ(fieldCount, 0u);

			ASSERT_EQ(instance->Invoke(), 3);
			ASSERT_EQ(callCount, 3);
		}


		// Basic tests for a callback with one unnamed argument
		TEST(SerializedActionTest, OneArgument_UnnamedArg) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);

			SerializedCallback action = SerializedCallback::Create<int>::From("Call", callback);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 1u);
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 2;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(nonEmptyNameFound);
			}
			ASSERT_EQ(counter, 0);
			instance->Invoke();
			ASSERT_EQ(counter, 2);

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 5;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(nonEmptyNameFound);
			}
			ASSERT_EQ(counter, 2);
			instance->Invoke();
			ASSERT_EQ(counter, 7);
		}

		// Basic tests for a function with one unnamed argument and a return value
		TEST(SerializedActionTest, OneArgument_UnnamedArg_ReturnValue) {
			int counter = 0;
			auto call = [&](int count) -> int { counter += count; return counter; };
			const Function<int, int> callback = Function<int, int>::FromCall(&call);

			SerializedAction<int> action = SerializedAction<int>::Create<int>::From("Call", callback);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(counter, 0);

			Reference<SerializedAction<int>::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 1u);
			ASSERT_EQ(counter, 0);

			ASSERT_EQ(instance->Invoke(), 0);
			ASSERT_EQ(counter, 0);

			ASSERT_EQ(instance->Invoke(), 0);
			ASSERT_EQ(counter, 0);

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 2;
					};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(nonEmptyNameFound);
			}
			ASSERT_EQ(counter, 0);
			ASSERT_EQ(instance->Invoke(), 2);
			ASSERT_EQ(counter, 2);

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 5;
					};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(nonEmptyNameFound);
			}
			ASSERT_EQ(counter, 2);
			ASSERT_EQ(instance->Invoke(), 7);
			ASSERT_EQ(counter, 7);
		}

		// Basic tests for a callback with one explicitly named argument
		TEST(SerializedActionTest, OneArgument_NamedArg) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);
			const std::string argName = "Count";

			SerializedAction action = SerializedCallback::Create<int>::From("Call", callback, argName);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 1u);
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != argName)
						incorrectNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 2;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(incorrectNameFound);
			}
			ASSERT_EQ(counter, 0);
			instance->Invoke();
			ASSERT_EQ(counter, 2);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != argName)
						incorrectNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 5;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(incorrectNameFound);
			}
			ASSERT_EQ(counter, 2);
			instance->Invoke();
			ASSERT_EQ(counter, 7);
		}

		// Basic tests for a callback with one argument that has a custom serializer
		TEST(SerializedActionTest, OneArgument_CustomSerializer) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);
			const Reference<const ValueSerializer<int>> serializer = ValueSerializer<int>::Create("Count!!!");

			SerializedAction action = SerializedCallback::Create<int>::From("Call", callback, serializer);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 1u);
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 0);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectSerializerFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer() != serializer)
						incorrectSerializerFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 2;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(incorrectSerializerFound);
			}
			ASSERT_EQ(counter, 0);
			instance->Invoke();
			ASSERT_EQ(counter, 2);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectSerializerFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer() != serializer)
						incorrectSerializerFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 5;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				ASSERT_TRUE(found);
				ASSERT_FALSE(nonIntFound);
				ASSERT_EQ(fieldCount, 1u);
				ASSERT_FALSE(incorrectSerializerFound);
			}
			ASSERT_EQ(counter, 2);
			instance->Invoke();
			ASSERT_EQ(counter, 7);
		}

		// Basic tests for a callback with two unnamed arguments
		TEST(SerializedActionTest, TwoArguments_UnnamedArgs) {
			int sumA = 0u;
			float sumB = 0u;
			auto call = [&](int a, float b) { sumA += a; sumB += b; };
			const Callback<int, float> callback = Callback<int, float>::FromCall(&call);

			SerializedAction action = SerializedCallback::Create<int, float>::From("Call", callback);

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 2u);
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);

			instance->Invoke();
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 2u);
			ASSERT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			ASSERT_TRUE(args[0u].Serializer()->TargetName() == "");
			ASSERT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			ASSERT_TRUE(args[1u].Serializer()->TargetName() == "");

			args[0u] = 1;
			instance->Invoke();
			ASSERT_EQ(sumA, 1);
			ASSERT_EQ(sumB, 0.0f);

			args[1u] = 4.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 2);
			ASSERT_EQ(sumB, 4.0f);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 4);
			ASSERT_LT(std::abs(sumB - 11.0f), std::numeric_limits<float>::epsilon());
		}
	}
}
