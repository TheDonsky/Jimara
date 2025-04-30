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

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(callCount, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 0u);
			EXPECT_EQ(callCount, 0);

			instance->Invoke();
			EXPECT_EQ(callCount, 1);

			instance->Invoke();
			EXPECT_EQ(callCount, 2);

			size_t fieldCount = 0u;
			auto examineField = [&](const SerializedObject& item) { fieldCount++; };
			instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			EXPECT_EQ(fieldCount, 0u);

			instance->Invoke();
			EXPECT_EQ(callCount, 3);
		}

		// Basic tests for a function return value with no arguments
		TEST(SerializedActionTest, NoArguments_ReturnValue) {
			int callCount = 0;
			auto call = [&]() -> int { callCount++; return callCount; };
			const Function<int> function = Function<int>::FromCall(&call);

			SerializedAction action = SerializedAction<int>::Create<>::From("Call", function);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(callCount, 0);

			Reference<SerializedAction<int>::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 0u);
			EXPECT_EQ(callCount, 0);

			EXPECT_EQ(instance->Invoke(), 1);
			EXPECT_EQ(callCount, 1);

			EXPECT_EQ(instance->Invoke(), 2);
			EXPECT_EQ(callCount, 2);

			size_t fieldCount = 0u;
			auto examineField = [&](const SerializedObject& item) { fieldCount++; };
			instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			EXPECT_EQ(fieldCount, 0u);

			EXPECT_EQ(instance->Invoke(), 3);
			EXPECT_EQ(callCount, 3);
		}


		// Basic tests for a callback with one unnamed argument
		TEST(SerializedActionTest, OneArgument_UnnamedArg) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);

			SerializedCallback action = SerializedCallback::Create<int>::From("Call", callback);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 0);
			instance->Invoke();
			EXPECT_EQ(counter, 2);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 2);
			instance->Invoke();
			EXPECT_EQ(counter, 7);
		}

		// Basic tests for a callback with one unnamed argument that is a referenced value
		TEST(SerializedActionTest, OneArgument_UnnamedArg_ReferenceValue) {
			int counter = 0;
			auto call = [&](const int& count) { counter += count; };
			const Callback<const int&> callback = Callback<const int&>::FromCall(&call);

			SerializedCallback action = SerializedCallback::Create<const int&>::From("Call", callback);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 0);
			instance->Invoke();
			EXPECT_EQ(counter, 2);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 2);
			instance->Invoke();
			EXPECT_EQ(counter, 7);
		}

		// Basic tests for a callback with one unnamed argument that is an enumeration value
		TEST(SerializedActionTest, OneArgument_UnnamedArg_EnumerationValue) {
			enum class Options : uint32_t {
				A,
				B,
				C, 
				D
			};

			Options curOpt = Options::A;
			auto call = [&](Options opt) { curOpt = opt; };
			const Callback<Options> callback = Callback<Options>::FromCall(&call);

			SerializedCallback action = SerializedCallback::Create<Options>::From("Call", callback);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(curOpt, Options::A);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(curOpt, Options::A);

			instance->Invoke();
			EXPECT_EQ(curOpt, Options::A);

			instance->Invoke();
			EXPECT_EQ(curOpt, Options::A);

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::UINT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 2u;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(curOpt, Options::A);
			instance->Invoke();
			EXPECT_EQ(curOpt, static_cast<Options>(2u));

			{
				bool found = false;
				bool nonIntFound = false;
				bool nonEmptyNameFound = false;
				size_t fieldCount = 0u;
				auto examineField = [&](const SerializedObject& item) {
					fieldCount++;
					if (item.Serializer()->TargetName() != "")
						nonEmptyNameFound = true;
					if (item.Serializer()->GetType() != ItemSerializer::Type::UINT_VALUE) {
						nonIntFound = true;
						return;
					}
					found = true;
					item = 5u;
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(curOpt, static_cast<Options>(2u));
			instance->Invoke();
			EXPECT_EQ(curOpt, static_cast<Options>(5u));
		}

		// Basic tests for a function with one unnamed argument and a return value
		TEST(SerializedActionTest, OneArgument_UnnamedArg_ReturnValue) {
			int counter = 0;
			auto call = [&](int count) -> int { counter += count; return counter; };
			const Function<int, int> callback = Function<int, int>::FromCall(&call);

			SerializedAction<int> action = SerializedAction<int>::Create<int>::From("Call", callback);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedAction<int>::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			EXPECT_EQ(instance->Invoke(), 0);
			EXPECT_EQ(counter, 0);

			EXPECT_EQ(instance->Invoke(), 0);
			EXPECT_EQ(counter, 0);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 0);
			EXPECT_EQ(instance->Invoke(), 2);
			EXPECT_EQ(counter, 2);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(nonEmptyNameFound);
			}
			EXPECT_EQ(counter, 2);
			EXPECT_EQ(instance->Invoke(), 7);
			EXPECT_EQ(counter, 7);
		}

		// Basic tests for a callback with one explicitly named argument
		TEST(SerializedActionTest, OneArgument_NamedArg) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);
			const std::string argName = "Count";

			SerializedAction action = SerializedCallback::Create<int>::From("Call", callback, argName);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectNameFound);
			}
			EXPECT_EQ(counter, 0);
			instance->Invoke();
			EXPECT_EQ(counter, 2);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectNameFound);
			}
			EXPECT_EQ(counter, 2);
			instance->Invoke();
			EXPECT_EQ(counter, 7);
		}

		// Basic tests for a callback with one argument that has a custom serializer
		TEST(SerializedActionTest, OneArgument_CustomSerializer) {
			int counter = 0;
			auto call = [&](int count) { counter += count; };
			const Callback<int> callback = Callback<int>::FromCall(&call);
			const Reference<const ValueSerializer<int>> serializer = ValueSerializer<int>::Create("Count!!!");

			SerializedAction action = SerializedCallback::Create<int>::From("Call", callback, serializer);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 0);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectSerializerFound);
			}
			EXPECT_EQ(counter, 0);
			instance->Invoke();
			EXPECT_EQ(counter, 2);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectSerializerFound);
			}
			EXPECT_EQ(counter, 2);
			instance->Invoke();
			EXPECT_EQ(counter, 7);
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

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(counter, 0);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(counter, 0);

			instance->Invoke();
			EXPECT_EQ(counter, 7);

			instance->Invoke();
			EXPECT_EQ(counter, 14);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectSerializerFound);
			}
			EXPECT_EQ(counter, 14);
			instance->Invoke();
			EXPECT_EQ(counter, 16);

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
				EXPECT_TRUE(found);
				EXPECT_FALSE(nonIntFound);
				EXPECT_EQ(fieldCount, 1u);
				EXPECT_FALSE(incorrectSerializerFound);
			}
			EXPECT_EQ(counter, 16);
			instance->Invoke();
			EXPECT_EQ(counter, 21);
		}

		// Basic tests for a callback with two unnamed arguments
		TEST(SerializedActionTest, TwoArguments_UnnamedArgs) {
			int sumA = 0u;
			float sumB = 0u;
			auto call = [&](int a, float b) { sumA += a; sumB += b; };
			const Callback<int, float> callback = Callback<int, float>::FromCall(&call);

			SerializedAction action = SerializedCallback::Create<int, float>::From("Call", callback);

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 2u);
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);

			instance->Invoke();
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 2u);
			EXPECT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			EXPECT_TRUE(args[0u].Serializer()->TargetName() == "");
			EXPECT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			EXPECT_TRUE(args[1u].Serializer()->TargetName() == "");

			args[0u] = 1;
			instance->Invoke();
			EXPECT_EQ(sumA, 1);
			EXPECT_EQ(sumB, 0.0f);

			args[1u] = 4.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 2);
			EXPECT_EQ(sumB, 4.0f);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 4);
			EXPECT_LT(std::abs(sumB - 11.0f), std::numeric_limits<float>::epsilon());
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

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 3u);
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);
			EXPECT_EQ(sumC, 0.0f);

			instance->Invoke();
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);
			EXPECT_EQ(sumC, 0.0f);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 3u);
			EXPECT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			EXPECT_TRUE(args[0u].Serializer()->TargetName() == "a");
			EXPECT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			EXPECT_TRUE(args[1u].Serializer()->TargetName() == "b");
			EXPECT_TRUE(args[2u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			EXPECT_TRUE(args[2u].Serializer()->TargetName() == "c");

			args[0u] = 1;
			instance->Invoke();
			EXPECT_EQ(sumA, 1);
			EXPECT_EQ(sumB, 0.0f);
			EXPECT_EQ(sumC, 0.0f);

			args[1u] = 4.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 2);
			EXPECT_EQ(sumB, 4.0f);
			EXPECT_EQ(sumC, 0.0f);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 4);
			EXPECT_LT(std::abs(sumB - 11.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 0.0f);

			args[0u] = 1;
			args[1u] = 7.0f;
			args[2u] = 5.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 5);
			EXPECT_LT(std::abs(sumB - 18.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 5.0f);
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

			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, 0u);

			Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 4u);
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, 0.0f);
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, 0u);

			instance->Invoke();
			EXPECT_EQ(sumA, 0);
			EXPECT_EQ(sumB, bDefault);
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, dDefault);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 4u);

			EXPECT_TRUE(args[0u].Serializer()->GetType() == ItemSerializer::Type::INT_VALUE);
			EXPECT_TRUE(args[0u].Serializer()->TargetName() == aName);
			EXPECT_TRUE(args[0u].Serializer()->TargetHint() == "");
			EXPECT_TRUE(args[0u].Serializer()->FindAttributeOfType<DefaultValueAttribute<int>>() == nullptr);

			EXPECT_TRUE(args[1u].Serializer()->GetType() == ItemSerializer::Type::FLOAT_VALUE);
			EXPECT_TRUE(args[1u].Serializer()->TargetName() == bName);
			EXPECT_TRUE(args[1u].Serializer()->TargetHint() == bHint);
			EXPECT_TRUE(args[1u].Serializer()->FindAttributeOfType<DefaultValueAttribute<float>>() != nullptr);
			EXPECT_TRUE(args[1u].Serializer()->FindAttributeOfType<DefaultValueAttribute<float>>()->value == bDefault);
			
			EXPECT_TRUE(args[2u].Serializer()->GetType() == ItemSerializer::Type::DOUBLE_VALUE);
			EXPECT_TRUE(args[2u].Serializer()->TargetName() == "c");
			EXPECT_TRUE(args[2u].Serializer()->TargetHint() == "");
			EXPECT_TRUE(args[2u].Serializer()->FindAttributeOfType<DefaultValueAttribute<double>>() == nullptr);

			EXPECT_TRUE(args[3u].Serializer()->GetType() == ItemSerializer::Type::UINT_VALUE);
			EXPECT_TRUE(args[3u].Serializer()->TargetName() == dName);
			EXPECT_TRUE(args[3u].Serializer()->TargetHint() == dHint);
			EXPECT_TRUE(args[3u].Serializer()->FindAttributeOfType<DefaultValueAttribute<uint32_t>>() != nullptr);
			EXPECT_TRUE(args[3u].Serializer()->FindAttributeOfType<DefaultValueAttribute<uint32_t>>()->value == dDefault);

			args[0u] = 1;
			instance->Invoke();
			EXPECT_EQ(sumA, 1);
			EXPECT_LT(std::abs(sumB - bDefault * 2.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, 2u * dDefault);

			args[1u] = 4.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 2);
			EXPECT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, 3u * dDefault);

			args[0u] = 2;
			args[1u] = 7.0f;
			instance->Invoke();
			EXPECT_EQ(sumA, 4);
			EXPECT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 0.0);
			EXPECT_EQ(sumD, 4u * dDefault);

			args[0u] = 0;
			args[1u] = 0.0f;
			args[2u] = 5.0;
			instance->Invoke();
			EXPECT_EQ(sumA, 4);
			EXPECT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 5.0);
			EXPECT_EQ(sumD, 5u * dDefault);

			args[0u] = 0;
			args[1u] = 0.0f;
			args[2u] = 0.0;
			args[3u] = 8u;
			instance->Invoke();
			EXPECT_EQ(sumA, 4);
			EXPECT_LT(std::abs(sumB - bDefault * 2.0f - 4.0f - 7.0f), std::numeric_limits<float>::epsilon());
			EXPECT_EQ(sumC, 5.0);
			EXPECT_EQ(sumD, 5u * dDefault + 8u);
		}

		// Basic tests for a callback where argument is a pointer
		TEST(SerializedActionTest, SingleArgument_ObjectPointer) {
			Object* ptr = nullptr;
			auto call = [&](Object* v) { ptr = v; };
			const Callback<Object*> callback = Callback<Object*>::FromCall(&call);

			const SerializedCallback action = SerializedCallback::Create<Object*>::From("Call", callback);
			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(ptr, nullptr);

			const Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(ptr, nullptr);

			instance->Invoke();
			EXPECT_EQ(ptr, nullptr);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 1u);
			EXPECT_EQ(ptr, nullptr);

			EXPECT_EQ(args[0u].Serializer()->GetType(), ItemSerializer::Type::OBJECT_PTR_VALUE);
			EXPECT_NE(dynamic_cast<const ItemSerializer::Of<Reference<Object>>*>(args[0u].Serializer()), nullptr);
			
			const Reference<Object> value = Object::Instantiate<Object>();
			EXPECT_EQ(value->RefCount(), 1u);

			args[0u].SetObjectValue(value);
			EXPECT_EQ(ptr, nullptr);
			EXPECT_EQ(value->RefCount(), 2u);

			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 2u);
		}

		// Basic tests for a callback where argument is an object-reference
		TEST(SerializedActionTest, SingleArgument_ObjectReference) {
			Object* ptr = nullptr;
			auto call = [&](Reference<Object> v) { ptr = v; };
			const Callback<Reference<Object>> callback = Callback<Reference<Object>>::FromCall(&call);

			const SerializedCallback action = SerializedCallback::Create<Reference<Object>>::From("Call", callback);
			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(ptr, nullptr);

			const Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(ptr, nullptr);

			instance->Invoke();
			EXPECT_EQ(ptr, nullptr);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 1u);
			EXPECT_EQ(ptr, nullptr);

			EXPECT_EQ(args[0u].Serializer()->GetType(), ItemSerializer::Type::OBJECT_PTR_VALUE);
			EXPECT_NE(dynamic_cast<const ItemSerializer::Of<Reference<Object>>*>(args[0u].Serializer()), nullptr);

			const Reference<Object> value = Object::Instantiate<Object>();
			EXPECT_EQ(value->RefCount(), 1u);

			args[0u].SetObjectValue(value);
			EXPECT_EQ(ptr, nullptr);
			EXPECT_EQ(value->RefCount(), 2u);

			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 2u);
		}

		namespace {
			struct TestWeakReferenceable : public virtual WeaklyReferenceable {
				struct WeakReferenceRestore : public virtual StrongReferenceProvider {
					TestWeakReferenceable* ptr = nullptr;
					virtual Reference<WeaklyReferenceable> RestoreStrongReference() { return ptr; }
				};

				const Reference<WeakReferenceRestore> m_restore;

				inline TestWeakReferenceable() : m_restore(Object::Instantiate<WeakReferenceRestore>()) {
					m_restore->ptr = this;
				}

				inline ~TestWeakReferenceable() {
					assert(m_restore->ptr == nullptr);
				}

				virtual void FillWeakReferenceHolder(WeakReferenceHolder& holder) override { holder = m_restore; }

				virtual void ClearWeakReferenceHolder(WeakReferenceHolder& holder) override { holder = nullptr; }

				virtual void OnOutOfScope()const override {
					m_restore->ptr = nullptr;
					WeaklyReferenceable::OnOutOfScope();
				}
			};
		}

		// Basic tests for a callback where argument is a weakly-referenceable object pointer
		TEST(SerializedActionTest, SingleArgument_WeakObjectReference) {
			TestWeakReferenceable* ptr = nullptr;
			auto call = [&](TestWeakReferenceable* v) { ptr = v; };
			const Callback<TestWeakReferenceable*> callback = Callback<TestWeakReferenceable*>::FromCall(&call);

			const SerializedCallback action = SerializedCallback::Create<TestWeakReferenceable*>::From("Call", callback);
			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(ptr, nullptr);

			const Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(ptr, nullptr);

			instance->Invoke();
			EXPECT_EQ(ptr, nullptr);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 1u);
			EXPECT_EQ(ptr, nullptr);

			EXPECT_EQ(args[0u].Serializer()->GetType(), ItemSerializer::Type::OBJECT_PTR_VALUE);
			EXPECT_NE(dynamic_cast<const ItemSerializer::Of<Reference<TestWeakReferenceable>>*>(args[0u].Serializer()), nullptr);

			Reference<TestWeakReferenceable> value = Object::Instantiate<TestWeakReferenceable>();
			EXPECT_EQ(value->RefCount(), 1u);

			{
				auto examineField = [&](const SerializedObject& item) { 
					item.SetObjectValue(value);
				};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			EXPECT_EQ(ptr, nullptr);
			EXPECT_EQ(value->RefCount(), 1u);

			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 1u);

			ptr = nullptr;
			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 1u);

			value = nullptr;
			instance->Invoke();
			EXPECT_EQ(ptr, nullptr);
		}

		// Basic tests for a callback where argument is a weakly-referenceable object reference
		TEST(SerializedActionTest, SingleArgument_WeakObjectStrongReference) {
			TestWeakReferenceable* ptr = nullptr;
			auto call = [&](Reference<TestWeakReferenceable> v) { ptr = v; };
			const Callback<Reference<TestWeakReferenceable>> callback = Callback<Reference<TestWeakReferenceable>>::FromCall(&call);

			const SerializedCallback action = SerializedCallback::Create<Reference<TestWeakReferenceable>>::From("Call", callback);
			EXPECT_EQ(action.Name(), "Call");
			EXPECT_EQ(ptr, nullptr);

			const Reference<SerializedCallback::Instance> instance = action.CreateInstance();
			ASSERT_NE(instance, nullptr);
			EXPECT_EQ(instance->ArgumentCount(), 1u);
			EXPECT_EQ(ptr, nullptr);

			instance->Invoke();
			EXPECT_EQ(ptr, nullptr);

			std::vector<SerializedObject> args = {};
			{
				auto examineField = [&](const SerializedObject& item) { args.push_back(item); };
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			ASSERT_EQ(args.size(), 1u);
			EXPECT_EQ(ptr, nullptr);

			EXPECT_EQ(args[0u].Serializer()->GetType(), ItemSerializer::Type::OBJECT_PTR_VALUE);
			EXPECT_NE(dynamic_cast<const ItemSerializer::Of<Reference<TestWeakReferenceable>>*>(args[0u].Serializer()), nullptr);

			Reference<TestWeakReferenceable> value = Object::Instantiate<TestWeakReferenceable>();
			EXPECT_EQ(value->RefCount(), 1u);

			{
				auto examineField = [&](const SerializedObject& item) {
					item.SetObjectValue(value);
					};
				instance->GetFields(Callback<SerializedObject>::FromCall(&examineField));
			}
			EXPECT_EQ(ptr, nullptr);
			EXPECT_EQ(value->RefCount(), 2u);

			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 2u);

			ptr = nullptr;
			instance->Invoke();
			EXPECT_EQ(ptr, value);
			EXPECT_EQ(value->RefCount(), 2u);

			value = nullptr;
			instance->Invoke();
			EXPECT_NE(ptr, nullptr);
		}


		namespace {
			struct SerializedActionTest_ValueObject : public virtual Object {
				int value = 0;
			};

			struct SerializedActionTest_BasicActionProvider : public virtual SerializedCallback::Provider {
				const Reference<SerializedActionTest_ValueObject> value = Object::Instantiate<SerializedActionTest_ValueObject>();

				static const constexpr std::string_view ADD_VALUE_NAME = "AddValue";

				void AddValue(int amount) { value->value += amount; }

				static const constexpr std::string_view SUBTRACT_VALUE_NAME = "SubtractValue";

				void SubtractValue(int amount) { value->value -= amount; }

				static const constexpr std::string_view ARGUMENT_NAME = "amount";

				virtual void GetSerializedActions(Callback<SerializedCallback> report) override {
					report(SerializedCallback::Create<int>::From(ADD_VALUE_NAME, Callback<int>(&SerializedActionTest_BasicActionProvider::AddValue, this), 
						SerializedCallback::FieldInfo<int> { ARGUMENT_NAME, "", 1 }));
					report(SerializedCallback::Create<int>::From(SUBTRACT_VALUE_NAME, Callback<int>(&SerializedActionTest_BasicActionProvider::SubtractValue, this),
						SerializedCallback::FieldInfo<int> { ARGUMENT_NAME, "", 3 }));
				}
			};

#pragma warning(disable: 4250)
			struct SerializedActionTest_WeakActionProvider 
				: public virtual SerializedActionTest_BasicActionProvider
				, public virtual TestWeakReferenceable {};
#pragma warning(default: 4250)
		}

		// Basic test for a ProvidedInstance with an expected strong reference
		TEST(SerializedActionTest, ProvidedInstance_Strong) {
			Reference<SerializedActionTest_BasicActionProvider> provider = Object::Instantiate<SerializedActionTest_BasicActionProvider>();
			ASSERT_NE(provider, nullptr);
			EXPECT_EQ(provider->RefCount(), 1u);

			const Reference<SerializedActionTest_ValueObject> value = provider->value;
			ASSERT_NE(value, nullptr);
			EXPECT_EQ(value->value, 0);

			SerializedCallback::ProvidedInstance instance = {};
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionProvider(provider);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 1);

			auto setValue = [&](int value) {
				EXPECT_EQ(1, 1);
				auto examineFn = [&](const Serialization::SerializedObject& ser) {
					if (ser.Serializer()->TargetName() != SerializedActionTest_BasicActionProvider::ARGUMENT_NAME ||
						ser.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE)
						return;
					ser = value;
				};
				instance.GetFields(Callback<SerializedObject>::FromCall(&examineFn));
			};

			setValue(4);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 1);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 5);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME, false);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 5);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 2);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME, true);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 2);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 2u);
			EXPECT_EQ(value->value, 5);

			provider = nullptr;
			EXPECT_NE(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(value->value, 5);

			instance.Invoke();
			EXPECT_NE(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(value->value, 8);
		}

		// Basic test for a ProvidedInstance with an expected weak reference
		TEST(SerializedActionTest, ProvidedInstance_Weak) {
			Reference<SerializedActionTest_BasicActionProvider> provider = Object::Instantiate<SerializedActionTest_WeakActionProvider>();
			ASSERT_NE(provider, nullptr);
			EXPECT_EQ(provider->RefCount(), 1u);

			const Reference<SerializedActionTest_ValueObject> value = provider->value;
			ASSERT_NE(value, nullptr);
			EXPECT_EQ(value->value, 0);

			SerializedCallback::ProvidedInstance instance = {};
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionProvider(provider);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), "");
			EXPECT_EQ(instance.ArgumentCount(), 0u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 0);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 1);

			auto setValue = [&](int value) {
				EXPECT_EQ(1, 1);
				auto examineFn = [&](const Serialization::SerializedObject& ser) {
					if (ser.Serializer()->TargetName() != SerializedActionTest_BasicActionProvider::ARGUMENT_NAME ||
						ser.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE)
						return;
					ser = value;
				};
				instance.GetFields(Callback<SerializedObject>::FromCall(&examineFn));
			};

			setValue(4);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 1);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 5);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME, false);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 5);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::SUBTRACT_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 2);

			instance.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME, true);
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 2);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), provider);
			EXPECT_EQ(instance.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
			EXPECT_EQ(instance.ArgumentCount(), 1u);
			EXPECT_EQ(provider->RefCount(), 1u);
			EXPECT_EQ(value->value, 5);

			provider = nullptr;
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(value->value, 5);

			instance.Invoke();
			EXPECT_EQ(instance.ActionProvider(), nullptr);
			EXPECT_EQ(value->value, 5);
		}

		// Basic test for a ProvidedInstance with an expected weak reference
		TEST(SerializedActionTest, ProvidedInstance_MoveAndCopy) {
			std::vector<Reference<SerializedActionTest_BasicActionProvider>> providers = {
				Object::Instantiate<SerializedActionTest_BasicActionProvider>(),
				Object::Instantiate<SerializedActionTest_WeakActionProvider>()
			};

			for (const Reference<SerializedActionTest_BasicActionProvider>& provider : providers) {

				const Reference<SerializedActionTest_ValueObject> value = provider->value;
				ASSERT_NE(value, nullptr);
				EXPECT_EQ(value->value, 0);

				size_t perInstRefCount = (dynamic_cast<WeaklyReferenceable*>(provider.operator->()) != nullptr) ? 0u : 1u;

				auto setValue = [&](SerializedCallback::ProvidedInstance& instance, int value) {
					EXPECT_EQ(1, 1);
					auto examineFn = [&](const Serialization::SerializedObject& ser) {
						if (ser.Serializer()->TargetName() != SerializedActionTest_BasicActionProvider::ARGUMENT_NAME ||
							ser.Serializer()->GetType() != ItemSerializer::Type::INT_VALUE)
							return;
						ser = value;
					};
					instance.GetFields(Callback<SerializedObject>::FromCall(&examineFn));
				};

				SerializedCallback::ProvidedInstance a = {};
				a.SetActionProvider(provider);
				EXPECT_EQ(provider->RefCount(), 1u + perInstRefCount);
				a.SetActionByName(SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
				EXPECT_EQ(a.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
				EXPECT_EQ(a.ArgumentCount(), 1u);
				setValue(a, 9);

				a.Invoke();
				EXPECT_EQ(value->value, 9);

				SerializedCallback::ProvidedInstance b(a);
				EXPECT_EQ(provider->RefCount(), 1u + 2u * perInstRefCount);
				EXPECT_EQ(b.ActionProvider(), provider);
				EXPECT_EQ(b.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
				EXPECT_EQ(b.ArgumentCount(), 1u);

				b.Invoke();
				EXPECT_EQ(value->value, 18);

				a.Invoke();
				EXPECT_EQ(a.ActionName(), SerializedActionTest_BasicActionProvider::ADD_VALUE_NAME);
				EXPECT_EQ(a.ArgumentCount(), 1u);
				EXPECT_EQ(value->value, 27);

				setValue(a, 2);
				a.Invoke();
				EXPECT_EQ(value->value, 29);

				b.Invoke();
				EXPECT_EQ(value->value, 38);

				setValue(b, 20);
				b.Invoke();
				EXPECT_EQ(value->value, 58);

				a.Invoke();
				EXPECT_EQ(value->value, 60);

				a = std::move(b);
				EXPECT_EQ(provider->RefCount(), 1u + perInstRefCount);
				EXPECT_EQ(a.ActionProvider(), provider);
				EXPECT_EQ(b.ActionProvider(), nullptr);
				b.Invoke();
				EXPECT_EQ(value->value, 60);
				a.Invoke();
				EXPECT_EQ(value->value, 80);

				SerializedCallback::ProvidedInstance c(std::move(a));
				EXPECT_EQ(provider->RefCount(), 1u + perInstRefCount);
				EXPECT_EQ(a.ActionProvider(), nullptr);
				EXPECT_EQ(b.ActionProvider(), nullptr);
				EXPECT_EQ(c.ActionProvider(), provider);
				a.Invoke();
				EXPECT_EQ(value->value, 80);
				b.Invoke();
				EXPECT_EQ(value->value, 80);
				c.Invoke();
				EXPECT_EQ(value->value, 100);

				a = c;
				EXPECT_EQ(provider->RefCount(), 1u + 2u * perInstRefCount);
				EXPECT_EQ(a.ActionProvider(), provider);
				EXPECT_EQ(b.ActionProvider(), nullptr);
				EXPECT_EQ(c.ActionProvider(), provider);
				a.Invoke();
				EXPECT_EQ(value->value, 120);
				b.Invoke();
				EXPECT_EQ(value->value, 120);
				c.Invoke();
				EXPECT_EQ(value->value, 140);

				setValue(a, 7);
				a.Invoke();
				EXPECT_EQ(value->value, 147);
				b.Invoke();
				EXPECT_EQ(value->value, 147);
				c.Invoke();
				EXPECT_EQ(value->value, 167);

				a = b;
				EXPECT_EQ(provider->RefCount(), 1u + perInstRefCount);
				EXPECT_EQ(a.ActionProvider(), nullptr);
				EXPECT_EQ(b.ActionProvider(), nullptr);
				EXPECT_EQ(c.ActionProvider(), provider);
				a.Invoke();
				EXPECT_EQ(value->value, 167);
				b.Invoke();
				EXPECT_EQ(value->value, 167);
				c.Invoke();
				EXPECT_EQ(value->value, 187);

				c = std::move(a);
				EXPECT_EQ(provider->RefCount(), 1u);
				EXPECT_EQ(a.ActionProvider(), nullptr);
				EXPECT_EQ(b.ActionProvider(), nullptr);
				EXPECT_EQ(c.ActionProvider(), nullptr);
				a.Invoke();
				EXPECT_EQ(value->value, 187);
				b.Invoke();
				EXPECT_EQ(value->value, 187);
				c.Invoke();
				EXPECT_EQ(value->value, 187);
			}
		}
	}
}
