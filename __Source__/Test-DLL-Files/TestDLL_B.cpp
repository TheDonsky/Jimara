#ifdef _WIN32
#define TEST_DLL_EXPORT __declspec(dllexport)
#else
#define TEST_DLL_EXPORT
#endif
#include <cstdint>
#include <Jimara/Data/Geometry/Mesh.h>

namespace TestDLL_B {
    class CustomTestClass : public virtual Jimara::Object {};
}

extern "C" {
    TEST_DLL_EXPORT const char* TestDLL_GetName() { return "DLL_B"; }

    TEST_DLL_EXPORT uint32_t TestDLL_GetMeshVertexCount(Jimara::Object* meshPtr) {
        return Jimara::TriMesh::Reader(dynamic_cast<Jimara::TriMesh*>(meshPtr)).VertCount();
    }

    TEST_DLL_EXPORT void TestDLL_RegisterCustomClass(bool yep) {
        static Jimara::Reference<Jimara::Object> registryEntry;
        if (yep) registryEntry = Jimara::TypeId::Of<TestDLL_B::CustomTestClass>().Register();
        else registryEntry = nullptr;
    }

    TEST_DLL_EXPORT size_t TestDLL_GetRegisteredTypeCount() {
        return Jimara::RegisteredTypeSet::Current()->Size();
    }
}
