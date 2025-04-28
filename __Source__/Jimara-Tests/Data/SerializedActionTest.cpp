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


		// Basic tests for a callback with one argument that has been described using an initializer-list
		TEST(SerializedActionTest, OneArgument_FieldInfo) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);
			static const constexpr std::string_view argName = "Count";
			static const constexpr std::string_view argHint = "Number to add";
			static const constexpr int defaultValue = 7;

			SerializedAction action = SerializedCallback::Create<int>::From(
				"Call", callback, 
				SerializedCallback::FieldInfo<int> { argName, argHint, defaultValue });

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 1u);
			ASSERT_EQ(counter, 0);

			instance->Invoke();
			ASSERT_EQ(counter, 7);

			instance->Invoke();
			ASSERT_EQ(counter, 14);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectSerializerFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != argName)
						incorrectSerializerFound = true;
					if (item.Serializer()->TargetHint() != argHint)
						incorrectSerializerFound = true;
					if (item.Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>() == nullptr ||
						item.Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>()->value != defaultValue)
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
			ASSERT_EQ(counter, 14);
			instance->Invoke();
			ASSERT_EQ(counter, 16);

			{
				bool found = false;
				bool nonIntFound = false;
				bool incorrectSerializerFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != argName)
						incorrectSerializerFound = true;
					if (item.Serializer()->TargetHint() != argHint)
						incorrectSerializerFound = true;
					if (item.Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>() == nullptr ||
						item.Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>()->value != defaultValue)
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
			ASSERT_EQ(counter, 16);
			instance->Invoke();
			ASSERT_EQ(counter, 21);
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

		// Basic tests for a callback with three arguments, that just received extra argument description in the constructor for no good reason
		TEST(SerializedActionTest, ThreeArguments_ExtraArg) {
			int sumA = 0u;
			float sumB = 0u;
			float sumC = 0u;
			auto call = [&](int a, float b, float c) { sumA += a; sumB += b; sumC += c; };
			const Callback<int, float, float> callback = Callback<int, float, float>::FromCall(&call);

			SerializedAction action = SerializedCallback::Create<int, float, float>
				::From("Call", callback, "a", "b", SerializedCallback::FieldInfo<float> { "c" }, "d");

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 3u);
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);
			ASSERT_EQ(sumC, 0.0f);

			instance->Invoke();
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);
			ASSERT_EQ(sumC, 0.0f);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 3u);
			ASSERT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			ASSERT_TRUE(args[0u].Serializer()->TargetName() == "a");
			ASSERT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			ASSERT_TRUE(args[1u].Serializer()->TargetName() == "b");
			ASSERT_TRUE(args[2u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			ASSERT_TRUE(args[2u].Serializer()->TargetName() == "c");

			args[0u] = 1;
			instance->Invoke();
			ASSERT_EQ(sumA, 1);
			ASSERT_EQ(sumB, 0.0f);
			ASSERT_EQ(sumC, 0.0f);

			args[1u] = 4.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 2);
			ASSERT_EQ(sumB, 4.0f);
			ASSERT_EQ(sumC, 0.0f);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 4);
			ASSERT_LT(std::abs(sumB - 11.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 0.0f);

			args[0u] = 1;
			args[1u] = 7.0f;
			args[2u] = 5.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 5);
			ASSERT_LT(std::abs(sumB - 18.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 5.0f);
		}

		// Basic tests for a callback with two unnamed arguments
		TEST(SerializedActionTest, FourArguments_MixedDescriptorTypes) {
			int sumA = 0u;
			float sumB = 0.0f;
			double sumC = 0.0;
			uint32_t sumD = 0u;
			auto call = [&](int a, float b, double c, uint32_t d) { sumA += a; sumB += b; sumC += c; sumD += d; };
			const Callback<int, float, double, uint32_t> callback = Callback<int, float, double, uint32_t>::FromCall(&call);

			static const constexpr std::string_view aName = "a";
			static const constexpr std::string_view bName = "b";
			static const constexpr std::string_view bHint = "bbbb";
			static const constexpr float bDefault = 1.0f;
			static const constexpr std::string_view dName = "d";
			static const constexpr std::string_view dHint = "dddd";
			static const constexpr uint32_t dDefault = 2u;


			SerializedAction action = SerializedCallback::Create<int, float, double, uint32_t>::From(
				"Call", callback,
				aName,
				DefaultSerializer<float>::Create(bName, bHint, { Object::Instantiate<DefaultValueAttribute<float>>(bDefault) }),
				"c",
				SerializedCallback::FieldInfo<uint32_t> { dName, dHint, dDefault },
				SerializedCallback::FieldInfo<std::string> {});

			ASSERT_EQ(action.Name(), "Call");
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, 0u);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			ASSERT_EQ(instance->ArgumentCount(), 4u);
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, 0.0f);
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, 0u);

			instance->Invoke();
			ASSERT_EQ(sumA, 0);
			ASSERT_EQ(sumB, bDefault);
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, dDefault);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 4u);

			ASSERT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			ASSERT_TRUE(args[0u].Serializer()->TargetName() == aName);
			ASSERT_TRUE(args[0u].Serializer()->TargetHint() == "");
			ASSERT_TRUE(args[0u].Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>() == nullptr);

			ASSERT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			ASSERT_TRUE(args[1u].Serializer()->TargetName() == bName);
			ASSERT_TRUE(args[1u].Serializer()->TargetHint() == bHint);
			ASSERT_TRUE(args[1u].Serializer()->FindAttributeOfType<DefaultValueAttribute<float>>() != nullptr);
			ASSERT_TRUE(args[1u].Serializer()->FindAttributeOfType<DefaultValueAttribute<float>>()->value == bDefault);
			
			ASSERT_TRUE(args[2u].Serializer()->GetType() == ItemSerializer::Type::DOUBLE_VALUE);
			ASSERT_TRUE(args[2u].Serializer()->TargetName() == "c");
			ASSERT_TRUE(args[2u].Serializer()->TargetHint() == "");
			ASSERT_TRUE(args[2u].Serializer()->FindAttributeOfType<DefaultValueAttribute<double>>() == nullptr);

			ASSERT_TRUE(args[3u].Serializer()->GetType() == ItemSerializer::Type::UINT_VALUE);
			ASSERT_TRUE(args[3u].Serializer()->TargetName() == dName);
			ASSERT_TRUE(args[3u].Serializer()->TargetHint() == dHint);
			ASSERT_TRUE(args[3u].Serializer()->FindAttributeOfType<DefaultValueAttribute<uint32_t>>() != nullptr);
			ASSERT_TRUE(args[3u].Serializer()->FindAttributeOfType<DefaultValueAttribute<uint32_t>>()->value == dDefault);

			args[0u] = 1;
			instance->Invoke();
			ASSERT_EQ(sumA, 1);
			ASSERT_LT(std::abs(sumB - bDefault * 2.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, 2u * dDefault);

			args[1u] = 4.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 2);
			ASSERT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, 3u * dDefault);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			ASSERT_EQ(sumA, 4);
			ASSERT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 0.0);
			ASSERT_EQ(sumD, 4u * dDefault);

			args[0u] = 0;
			args[1u] = 0.0f;
			args[2u] = 5.0;
			instance->Invoke();
			ASSERT_EQ(sumA, 4);
			ASSERT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 5.0);
			ASSERT_EQ(sumD, 5u * dDefault);

			args[0u] = 0;
			args[1u] = 0.0f;
			args[2u] = 0.0;
			args[3u] = 8u;
			instance->Invoke();
			ASSERT_EQ(sumA, 4);
			ASSERT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			ASSERT_EQ(sumC, 5.0);
			ASSERT_EQ(sumD, 5u * dDefault + 8u);
		}
	}
}
