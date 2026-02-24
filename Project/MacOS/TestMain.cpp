#include <gtest/gtest.h>
#include <Jimara/OS/System/MainThreadCallbacks.h>

int main(int argc, char** argv) {
    return Jimara::OS::MainThreadCallbacks::RunOnSecondaryThread([](int count, char** args) {
        testing::InitGoogleTest(&count, args);
        return RUN_ALL_TESTS();
    }, argc, argv);
}
