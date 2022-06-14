#ifdef _WIN32
#define TEST_DLL_EXPORT __declspec(dllexport)
#else
#define TEST_DLL_EXPORT
#endif
#include <cstdint>


extern "C" {
    TEST_DLL_EXPORT const char* TestDLL_GetName() { return "DLL_B"; }
}
