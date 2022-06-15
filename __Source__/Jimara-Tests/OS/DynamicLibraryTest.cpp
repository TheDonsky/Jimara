#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include <OS/System/DynamicLibrary.h>
#include <Data/Generators/MeshConstants.h>
#include <thread>


namespace Jimara {
	namespace OS {
		// Test for basic DynamicLibrary::Load
		TEST(DynamicLibraryTest, Load) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				EXPECT_NE(library, nullptr);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_B") + DynamicLibrary::FileExtension().data(), logger);
				EXPECT_NE(library, nullptr);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> libraryA = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				Reference<DynamicLibrary> libraryB = DynamicLibrary::Load(std::string("TestDLL_B") + DynamicLibrary::FileExtension().data(), logger);
				EXPECT_NE(libraryA, nullptr);
				EXPECT_NE(libraryB, nullptr);
				EXPECT_NE(libraryA, libraryB);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> library_0 = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				Reference<DynamicLibrary> library_1 = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				EXPECT_NE(library_0, nullptr);
				EXPECT_NE(library_1, nullptr);
				EXPECT_EQ(library_0, library_1);
				EXPECT_EQ(library_0->RefCount(), 2);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_C") + DynamicLibrary::FileExtension().data(), logger);
				EXPECT_EQ(library, nullptr);
				EXPECT_NE(logger->Numfailures(), 0);
			}
		}

		// Test for basic DynamicLibrary::FunctionPtr
		TEST(DynamicLibraryTest, FunctionPointers) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				ASSERT_NE(library, nullptr);
				uint32_t(*getVal)() = library->GetFunction<uint32_t>("TestDLL_A_Get77773");
				ASSERT_NE(getVal, nullptr);
				EXPECT_EQ(getVal(), 77773);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> libraryA = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				Reference<DynamicLibrary> libraryB = DynamicLibrary::Load(std::string("TestDLL_B") + DynamicLibrary::FileExtension().data(), logger);
				ASSERT_NE(libraryA, nullptr);
				ASSERT_NE(libraryB, nullptr);
				const char* (*getNameA)() = libraryA->GetFunction<const char*>("TestDLL_GetName");
				const char* (*getNameB)() = libraryB->GetFunction<const char*>("TestDLL_GetName");
				ASSERT_NE(getNameA, nullptr);
				ASSERT_NE(getNameB, nullptr);
				EXPECT_EQ(std::string_view(getNameA()), "DLL_A");
				EXPECT_EQ(std::string_view(getNameB()), "DLL_B");
				EXPECT_EQ(logger->Numfailures(), 0);
			}
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				ASSERT_NE(library, nullptr);
				void(*nonExistantFn)() = library->GetFunction<void>("TestDLL_NON_EXISTANT_FN");
				EXPECT_EQ(nonExistantFn, nullptr);
				EXPECT_GT(logger->Numfailures(), 0);
			}
		}

		// Test for basic DynamicLibrary::Load with no extension
		TEST(DynamicLibraryTest, NoExtension) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			{
				Reference<DynamicLibrary> library = DynamicLibrary::Load("TestDLL_A", logger);
				ASSERT_NE(library, nullptr);
				uint32_t(*getVal)() = library->GetFunction<uint32_t>("TestDLL_A_Get77773");
				ASSERT_NE(getVal, nullptr);
				EXPECT_EQ(getVal(), 77773);
				EXPECT_EQ(logger->Numfailures(), 0);
			}
		}

		// Test for DLLMain, thread local storage and destructors
		TEST(DynamicLibraryTest, Lifecycle) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			for (size_t i = 0; i < 4; i++) {
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				ASSERT_NE(library, nullptr);
				std::string_view(*getState)() = library->GetFunction<std::string_view>("TestDLL_InitializationState");
				ASSERT_NE(getState, nullptr);
				EXPECT_EQ(getState(), "DLL_A INITIALIZED");
				EXPECT_EQ(logger->Numfailures(), 0);
			}

			for (size_t i = 0; i < 4; i++) {
				Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
				ASSERT_NE(library, nullptr);

				std::vector<std::thread> threads;
				std::vector<std::vector<int>> values;
				values.resize(16);
				static const constexpr size_t queriesPerThread = 4096;
				for (size_t i = 0; i < values.size(); i++) {
					std::vector<int>& v = values[i];
					for (size_t j = 0; j < queriesPerThread; j++)
						v.push_back(64);
					threads.push_back(std::thread([](DynamicLibrary* lib, volatile int* v) {
						int(*getValue)() = lib->GetFunction<int>("TestDLL_ThreadLocalCounter");
						for (size_t j = 0; j < queriesPerThread; j++)
							v[j] = getValue();
						}, library.operator->(), v.data()));
				}
				int(*getValue)() = library->GetFunction<int>("TestDLL_ThreadLocalCounter");
				ASSERT_NE(getValue, nullptr);
				EXPECT_EQ(getValue(), 0);

				for (size_t i = 0; i < values.size(); i++) 
					threads[i].join();

				for (size_t i = 0; i < values.size(); i++) {
					const std::vector<int>& v = values[i];
					EXPECT_EQ(v.size(), queriesPerThread);
					std::stringstream actual;
					std::stringstream expected;
					for (size_t j = 0; j < v.size(); j++) {
						expected << j << " ";
						actual << v[j] << " ";
					}
					const std::string got = actual.str();
					const std::string wanted = expected.str();
					EXPECT_STREQ(got.c_str(), wanted.c_str());
				}
				EXPECT_EQ(logger->Numfailures(), 0);
			}

			for (size_t i = 0; i < 16; i++) {
				uint32_t count = 0;
				typedef void(*ExecuteFn)(void*);
				const ExecuteFn execute = [](void* ptr) { ((uint32_t*)ptr)[0]++; };

				for (size_t i = 0; i < 4; i++) {
					EXPECT_EQ(count, i);
					Reference<DynamicLibrary> library = DynamicLibrary::Load(std::string("TestDLL_A") + DynamicLibrary::FileExtension().data(), logger);
					ASSERT_NE(library, nullptr);
					void(*onUnload)(ExecuteFn, void*) = library->GetFunction<void, ExecuteFn, void*>("TestDLL_ExecuteOnUnload");
					ASSERT_NE(onUnload, nullptr);
					onUnload(execute, &count);
					library = nullptr;
					EXPECT_EQ(count, i + 1);
				}
				EXPECT_EQ(logger->Numfailures(), 0);
			}
		}

		class DynamicLibraryTest_EngineLink_TestClass : public virtual Object {};

		// Test for engine linkage
		TEST(DynamicLibraryTest, EngineLink) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			Reference<DynamicLibrary> library = DynamicLibrary::Load("TestDLL_B", logger);
			ASSERT_NE(library, nullptr);

			{
				uint32_t(*getMeshVertexCount)(Object*) = library->GetFunction<uint32_t, Object*>("TestDLL_GetMeshVertexCount");
				ASSERT_NE(getMeshVertexCount, nullptr);
				Reference<TriMesh> box = MeshConstants::Tri::Cube();
				EXPECT_EQ(getMeshVertexCount(box), TriMesh::Reader(box).VertCount());
			}

			{
				void(*registerCustomClass)(bool) = library->GetFunction<void, bool>("TestDLL_RegisterCustomClass");
				ASSERT_NE(registerCustomClass, nullptr);

				size_t(*getRegisteredTypeCount)() = library->GetFunction<size_t>("TestDLL_GetRegisteredTypeCount");
				ASSERT_NE(getRegisteredTypeCount, nullptr);
				
				const size_t initialCount = RegisteredTypeSet::Current()->Size();
				registerCustomClass(true);
				const size_t countAfterRegistration = RegisteredTypeSet::Current()->Size();
				EXPECT_EQ(initialCount + 1, countAfterRegistration);
				{
					TypeId id;
					EXPECT_TRUE(TypeId::Find("TestDLL_B::CustomTestClass", id));
					EXPECT_EQ(id.Name(), "TestDLL_B::CustomTestClass");
				}
				EXPECT_EQ(getRegisteredTypeCount(), RegisteredTypeSet::Current()->Size());

				registerCustomClass(false);
				const size_t countAfterUnregistration = RegisteredTypeSet::Current()->Size();
				EXPECT_EQ(initialCount, countAfterUnregistration);
				EXPECT_EQ(countAfterUnregistration + 1, countAfterRegistration);
				{
					TypeId id;
					EXPECT_FALSE(TypeId::Find("TestDLL_B::CustomTestClass", id));
				}
				EXPECT_EQ(getRegisteredTypeCount(), RegisteredTypeSet::Current()->Size());
			}

			{
				size_t(*getRegisteredTypeCount)() = library->GetFunction<size_t>("TestDLL_GetRegisteredTypeCount");
				ASSERT_NE(getRegisteredTypeCount, nullptr);

				const size_t initialCount = RegisteredTypeSet::Current()->Size();
				Reference<Object> registryEntry = TypeId::Of<DynamicLibraryTest_EngineLink_TestClass>().Register();
				EXPECT_NE(registryEntry, nullptr);
				EXPECT_EQ(initialCount + 1, RegisteredTypeSet::Current()->Size());
				static const constexpr std::string_view name = "Jimara::OS::DynamicLibraryTest_EngineLink_TestClass";
				static_assert(TypeId::Of<DynamicLibraryTest_EngineLink_TestClass>().Name() == name);
				{
					TypeId id;
					EXPECT_TRUE(TypeId::Find(name, id));
					EXPECT_EQ(id.Name(), name);
				}
				EXPECT_EQ(getRegisteredTypeCount(), RegisteredTypeSet::Current()->Size());

				registryEntry = nullptr;
				{
					TypeId id;
					EXPECT_FALSE(TypeId::Find(name, id));
				}
				EXPECT_EQ(getRegisteredTypeCount(), RegisteredTypeSet::Current()->Size());
			}
		}
	}
}
