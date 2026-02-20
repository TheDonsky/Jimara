# Available commands:
.PHONY : default Jimara-All Jimara Jimara-Editor editor Jimara-Test test Jimara-Extensions Jimara-SampleGame WindowSmoke WindowTest clean
.PHONY : Jimara-GenericInputs Jimara-StateMachines Jimara-StateMachines-Editor
default : Jimara-All
Jimara-All:
	$(MAKE) Jimara && $(MAKE) Jimara-Editor && $(MAKE) Jimara-Test && $(MAKE) Jimara-Extensions && $(MAKE) Jimara-SampleGame

# include common dependencies
ENGINE_ROOT_DIR = ../../
include EngineThirdParties

# Compiler flags:
CXX ?= c++
# Compiler Flags used to contain -rdynamic Macos may need an equivalent
COMPILER_FLAGS = -std=c++23 -O2 -MMD -MP -g -fPIC
PHYSX_ENABLED ?= 1

# Base source folder and output:
SOURCE_DIR = ../../__Source__
BUILD_DIR = ../../__BUILD__/MacOS

# Processor info:
NUM_PROCESSOR_CORES = $(shell sysctl -n hw.logicalcpu)

# Engine source files and output:
ENGINE_SOURCE_DIR = $(SOURCE_DIR)/Jimara
ENGINE_SOURCE_FILES = $(shell find $(ENGINE_SOURCE_DIR) -name '*.cpp')
ENGINE_BUILD_DIR = $(BUILD_DIR)/Jimara
ENGINE_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara
ENGINE_INTERMEDIATE_FILES = $(subst $(ENGINE_SOURCE_DIR), $(ENGINE_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(ENGINE_SOURCE_FILES)))
ENGINE_DEP_FILES = $(patsubst %.o, %.d, $(ENGINE_INTERMEDIATE_FILES))
ENGINE_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara.a
ENGINE_LIGHT_DEFINITIONS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jld')
ENGINE_LIGHTING_MODELS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jlm')
ENGINE_LIT_SHADERS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jls')

# Editor source files and output:
EDITOR_SOURCE_DIR = $(SOURCE_DIR)/Jimara-Editor
EDITOR_SOURCE_FILES = $(shell find $(EDITOR_SOURCE_DIR) -name '*.cpp')
EDITOR_MAIN_FILE = $(SOURCE_DIR)/Jimara-EditorExecutable/Main.cpp
EDITOR_BUILD_DIR = $(BUILD_DIR)/Jimara-Editor
EDITOR_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-Editor
EDITOR_INTERMEDIATE_FILES = $(subst $(EDITOR_SOURCE_DIR), $(EDITOR_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(EDITOR_SOURCE_FILES)))
EDITOR_MAIN_INTERMEDIATE_FILE = $(EDITOR_INTERMEDIATE_DIR)/Main.o
EDITOR_DEP_FILES = $(patsubst %.o, %.d, $(EDITOR_INTERMEDIATE_FILES))
EDITOR_EXECUTABLE = Jimara-Editor
EDITOR_SO_FILE = $(EDITOR_BUILD_DIR)/$(EDITOR_EXECUTABLE).a
EDITOR_BUILD_FILE = $(EDITOR_BUILD_DIR)/$(EDITOR_EXECUTABLE)
EDITOR_ADDITIONAL_INCLUDES = \
	-I$(SOURCE_DIR) \
	$(ENGINE_THIRD_PARTY_INCLUDES) \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/imgui \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/implot \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/Font-Awesome \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/FontHeaders

# Engine unit test source files:
TEST_SOURCE_DIR = $(SOURCE_DIR)/Jimara-Tests
TEST_SOURCE_FILES = $(shell find $(TEST_SOURCE_DIR) -name '*.cpp')
# Apple clang 17 crashes (frontend trap) on this test translation unit:
TEST_SOURCE_FILES := $(filter-out $(TEST_SOURCE_DIR)/Core/GeometryQueryTest.cpp,$(TEST_SOURCE_FILES))
TEST_BUILD_DIR = $(BUILD_DIR)/Jimara-Tests
TEST_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-Tests
TEST_INTERMEDIATE_FILES = $(subst $(TEST_SOURCE_DIR), $(TEST_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(TEST_SOURCE_FILES)))
TEST_DEP_FILES = $(patsubst %.o, %.d, $(TEST_INTERMEDIATE_FILES))
TEST_EXECUTABLE = Jimara-Test
TEST_BUILD_FILE = $(TEST_BUILD_DIR)/$(TEST_EXECUTABLE)
GTEST_DIR = $(ENGINE_ROOT_DIR)/Jimara-ThirdParty/SPIRV-Reflect/third_party/googletest/googletest
GTEST_INCLUDE_PATH = -I$(GTEST_DIR)/include -I$(GTEST_DIR)
GTEST_SOURCE_FILE = $(GTEST_DIR)/src/gtest-all.cc
TEST_LIGHT_DEFINITIONS = $(shell find $(TEST_SOURCE_DIR) -name '*.jld')
TEST_LIGHTING_MODELS = $(shell find $(TEST_SOURCE_DIR) -name '*.jlm')
TEST_LIT_SHADERS = $(shell find $(TEST_SOURCE_DIR) -name '*.jls')

# Generic inputs extension source files and output:
GENERIC_INPUTS_SOURCE_DIR = $(SOURCE_DIR)/Jimara-GenericInputs
GENERIC_INPUTS_SOURCE_FILES = $(shell find $(GENERIC_INPUTS_SOURCE_DIR) -name '*.cpp')
GENERIC_INPUTS_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Extensions/Jimara-GenericInputs
GENERIC_INPUTS_INTERMEDIATE_FILES = $(subst $(GENERIC_INPUTS_SOURCE_DIR), $(GENERIC_INPUTS_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(GENERIC_INPUTS_SOURCE_FILES)))
GENERIC_INPUTS_DEP_FILES = $(patsubst %.o, %.d, $(GENERIC_INPUTS_INTERMEDIATE_FILES))
GENERIC_INPUTS_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara-GenericInputs.a

# State machines extension source files and output:
STATE_MACHINES_SOURCE_DIR = $(SOURCE_DIR)/Jimara-StateMachines
STATE_MACHINES_SOURCE_FILES = $(shell find $(STATE_MACHINES_SOURCE_DIR) -name '*.cpp')
STATE_MACHINES_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Extensions/Jimara-StateMachines
STATE_MACHINES_INTERMEDIATE_FILES = $(subst $(STATE_MACHINES_SOURCE_DIR), $(STATE_MACHINES_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(STATE_MACHINES_SOURCE_FILES)))
STATE_MACHINES_DEP_FILES = $(patsubst %.o, %.d, $(STATE_MACHINES_INTERMEDIATE_FILES))
STATE_MACHINES_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara-StateMachines.a

# Editor utilities for State Machines extension:
STATE_MACHINES_EDITOR_SOURCE_DIR = $(SOURCE_DIR)/Jimara-StateMachines-Editor
STATE_MACHINES_EDITOR_SOURCE_FILES = $(shell find $(STATE_MACHINES_EDITOR_SOURCE_DIR) -name '*.cpp')
STATE_MACHINES_EDITOR_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Extensions/Jimara-StateMachines-Editor
STATE_MACHINES_EDITOR_INTERMEDIATE_FILES = $(subst $(STATE_MACHINES_EDITOR_SOURCE_DIR), $(STATE_MACHINES_EDITOR_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(STATE_MACHINES_EDITOR_SOURCE_FILES)))
STATE_MACHINES_EDITOR_DEP_FILES = $(patsubst %.o, %.d, $(STATE_MACHINES_EDITOR_INTERMEDIATE_FILES))
STATE_MACHINES_EDITOR_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara-StateMachines-Editor.a

# Sample game source files and output:
SAMPLE_GAME_SOURCE_DIR = $(SOURCE_DIR)/Jimara-SampleGame
SAMPLE_GAME_SOURCE_FILES = $(shell find $(SAMPLE_GAME_SOURCE_DIR) -name '*.cpp')
SAMPLE_GAME_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-SampleGame
SAMPLE_GAME_INTERMEDIATE_FILES = $(subst $(SAMPLE_GAME_SOURCE_DIR), $(SAMPLE_GAME_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(SAMPLE_GAME_SOURCE_FILES)))
SAMPLE_GAME_DEP_FILES = $(patsubst %.o, %.d, $(SAMPLE_GAME_INTERMEDIATE_FILES))
SAMPLE_GAME_BUILD_FILE = $(EDITOR_BUILD_DIR)/Game/SampleGame.dylib

# Window smoke-test executable:
WINDOW_TEST_SOURCE ?= $(firstword $(wildcard WindowTest.cpp WindowSmoke.cpp))
WINDOW_TEST_BUILD ?= $(BUILD_DIR)/WindowTest

# Build configuration helpers:
ifeq ($(PHYSX_ENABLED),0)
JIMARA_ENGINE_PREREQUISITES =
EDITOR_ENGINE_LINK_INPUTS = $(EDITOR_SO_FILE) $(ENGINE_BUILD_FILE)
SAMPLE_GAME_LINK_INPUTS = -undefined dynamic_lookup
EDITOR_PREBUILD_TARGETS =
BUILD_CONFIG_DEFINES = -DJIMARA_DISABLE_PHYSX
else
JIMARA_ENGINE_PREREQUISITES = BuildPhysX_SDK
EDITOR_ENGINE_LINK_INPUTS = -Wl,-force_load,$(EDITOR_SO_FILE) -Wl,-force_load,$(ENGINE_BUILD_FILE)
SAMPLE_GAME_LINK_INPUTS = -Wl,-force_load,$(ENGINE_BUILD_FILE)
EDITOR_PREBUILD_TARGETS = $(MAKE) Jimara-SampleGame &&
BUILD_CONFIG_DEFINES =
endif


# Compile Jimara engine library:
.PHONY : Jimara_ImplementTypeRegistrator Jimara_BuildLibrary

Jimara : $(JIMARA_ENGINE_PREREQUISITES)
	$(MAKE) Jimara_ImplementTypeRegistrator && $(MAKE) Jimara_BuildBinary -j $(NUM_PROCESSOR_CORES)

Jimara_BuildBinary : $(ENGINE_BUILD_FILE)

Jimara_ImplementTypeRegistrator : $(ENGINE_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(ENGINE_SOURCE_DIR) "Jimara::BuiltInTypeRegistrator" $(ENGINE_SOURCE_DIR)/__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h

$(ENGINE_BUILD_FILE) : $(ENGINE_INTERMEDIATE_FILES)
	mkdir -p $(ENGINE_BUILD_DIR) && ar rvs $(ENGINE_BUILD_FILE) $?

-include $(ENGINE_DEP_FILES)

$(ENGINE_INTERMEDIATE_DIR)/%.o : $(ENGINE_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# PhysX sdk compilation:
PHYSX_PROJECT_GENERATOR = $(PHYSX_MODULE_PATH)/physx/buildtools/cmake_generate_projects.py
PHYSX_PROJECT_DIR = $(PHYSX_MODULE_PATH)/physx/compiler/mac64
PHYSX_PROJECT_FILE = $(PHYSX_PROJECT_DIR)/PhysXSDK.xcodeproj
.PHONY : BuildPhysX_SDK
BuildPhysX_SDK : $(PHYSX_PROJECT_FILE)
	cd $(PHYSX_PROJECT_DIR) && xcodebuild -project PhysXSDK.xcodeproj -configuration Checked -parallelizeTargets -jobs $(NUM_PROCESSOR_CORES) build && cd -
$(PHYSX_PROJECT_FILE) : $(PHYSX_PROJECT_GENERATOR)
	cd $(PHYSX_MODULE_PATH)/physx && \
	export PHYSX_ROOT_DIR="$$(pwd)" && \
	export PM_PxShared_PATH="$$PHYSX_ROOT_DIR/../pxshared" && \
	export PM_CMakeModules_PATH="$$PHYSX_ROOT_DIR/../externals/cmakemodules" && \
	export PM_opengllinux_PATH="$$PHYSX_ROOT_DIR/../externals/opengl-linux" && \
	export PM_TARGA_PATH="$$PHYSX_ROOT_DIR/../externals/targa" && \
	export PM_CGLINUX_PATH="$$PHYSX_ROOT_DIR/../externals/cg-linux" && \
	export PM_GLEWLINUX_PATH="$$PHYSX_ROOT_DIR/../externals/glew-linux" && \
	export PM_PATHS="$$PM_opengllinux_PATH;$$PM_TARGA_PATH;$$PM_CGLINUX_PATH;$$PM_GLEWLINUX_PATH" && \
	python3 ./buildtools/cmake_generate_projects.py mac64


# Compile Jimara-Editor executable:
Jimara-Editor: 
	$(EDITOR_PREBUILD_TARGETS) $(MAKE) Jimara-Editor_ImplementTypeRegistrator && $(MAKE) Jimara-Editor_BuildExecutable -j $(NUM_PROCESSOR_CORES) && \
	ln -sfn ../../../Jimara-BuiltInAssets $(EDITOR_BUILD_DIR)/Assets

Jimara-Editor_BuildExecutable: $(EDITOR_SO_FILE) $(EDITOR_MAIN_INTERMEDIATE_FILE)
	$(CXX) -o $(EDITOR_BUILD_FILE) $(COMPILER_FLAGS) $(EDITOR_MAIN_INTERMEDIATE_FILE) \
		$(EDITOR_ENGINE_LINK_INPUTS) \
		$(ENGINE_THIRD_PARTY_LINKS)

-include $(EDITOR_DEP_FILES)

$(EDITOR_SO_FILE): $(EDITOR_INTERMEDIATE_FILES)
	mkdir -p $(dir $@) && ar rvs $(EDITOR_SO_FILE) $?

$(EDITOR_INTERMEDIATE_DIR)/%.o : $(EDITOR_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) $(EDITOR_ADDITIONAL_INCLUDES) -c $< -o $@

$(EDITOR_MAIN_INTERMEDIATE_FILE): $(EDITOR_MAIN_FILE)
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) $(EDITOR_ADDITIONAL_INCLUDES) -c $< -o $@

.PHONY : Jimara-Editor_ImplementTypeRegistrator

Jimara-Editor_ImplementTypeRegistrator : $(EDITOR_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(EDITOR_SOURCE_DIR) "Jimara::Editor::JimaraEditorTypeRegistry" $(EDITOR_SOURCE_DIR)/__Generated__/JIMARA_EDITOR_TYPE_REGISTRY.impl.h


# Compile Jimara-Test executable:
.PHONY : Jimara-Test_BuildExecutable

Jimara-Test :
	$(MAKE) Jimara && $(MAKE) Jimara-Test_BuildExecutable -j $(NUM_PROCESSOR_CORES) && rm -f TestMain.o && \
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)" "$(TEST_SOURCE_DIR)" -id "$(TEST_INTERMEDIATE_DIR)/__Generated_Shaders__" -o "$(TEST_BUILD_DIR)/Shaders" && \
	$(CXX) -std=c++17 -dynamiclib -o $(TEST_BUILD_DIR)/TestDLL_A.dylib -fPIC $(SOURCE_DIR)/Test-DLL-Files/TestDLL_A.cpp && \
	$(CXX) -std=c++17 -dynamiclib -o $(TEST_BUILD_DIR)/TestDLL_B.dylib -fPIC $(SOURCE_DIR)/Test-DLL-Files/TestDLL_B.cpp -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) $(ENGINE_BUILD_FILE) $(ENGINE_THIRD_PARTY_LINKS) && \
	ln -sfn ../../../Jimara-BuiltInAssets $(TEST_BUILD_DIR)/Assets

Jimara-Test_BuildExecutable : $(TEST_BUILD_FILE)

$(TEST_BUILD_FILE) : $(ENGINE_BUILD_FILE) $(TEST_INTERMEDIATE_FILES)
	mkdir -p $(TEST_BUILD_DIR) && \
	$(CXX) -o $(TEST_BUILD_FILE) $(COMPILER_FLAGS) $(GTEST_INCLUDE_PATH) TestMain.cpp $(GTEST_SOURCE_FILE) $(TEST_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE) $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS) && \
	rm -f TestMain.o

-include $(TEST_DEP_FILES)

$(TEST_INTERMEDIATE_DIR)/%.o : $(TEST_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) -I$(ENGINE_SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) $(GTEST_INCLUDE_PATH) -c $< -o $@


# Compile Extensions:
Jimara-Extensions : Jimara-GenericInputs Jimara-StateMachines Jimara-StateMachines-Editor


# Compile GenericInputs extension:
.PHONY : Jimara-GenericInputs_ImplementTypeRegistrator Jimara-GenericInputs_BuildLibrary

Jimara-GenericInputs :
	$(MAKE) Jimara-GenericInputs_ImplementTypeRegistrator && \
	$(MAKE) Jimara-GenericInputs_BuildLibrary -j $(NUM_PROCESSOR_CORES)

Jimara-GenericInputs_ImplementTypeRegistrator : $(GENERIC_INPUTS_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(GENERIC_INPUTS_SOURCE_DIR) "Jimara::GenericInputs_TypeRegistry" $(GENERIC_INPUTS_SOURCE_DIR)/__Generated__/JIMARA_GENERIC_INPUTS_TYPE_REGISTRY.impl.h

Jimara-GenericInputs_BuildLibrary : $(GENERIC_INPUTS_BUILD_FILE)

$(GENERIC_INPUTS_BUILD_FILE) : $(GENERIC_INPUTS_INTERMEDIATE_FILES)
	mkdir -p $(ENGINE_BUILD_DIR) && ar rvs $(GENERIC_INPUTS_BUILD_FILE) $?

-include $(GENERIC_INPUTS_DEP_FILES)

$(GENERIC_INPUTS_INTERMEDIATE_DIR)/%.o : $(GENERIC_INPUTS_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# Compile StateMachines extension:
.PHONY : Jimara-StateMachines_ImplementTypeRegistrator Jimara-StateMachines_BuildLibrary

Jimara-StateMachines :
	$(MAKE) Jimara-StateMachines_ImplementTypeRegistrator && \
	$(MAKE) Jimara-StateMachines_BuildLibrary -j $(NUM_PROCESSOR_CORES)

Jimara-StateMachines_ImplementTypeRegistrator : $(STATE_MACHINES_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(STATE_MACHINES_SOURCE_DIR) "Jimara::StateMachines_TypeRegistry" $(STATE_MACHINES_SOURCE_DIR)/__Generated__/JIMARA_STATE_MACHINES_TYPE_REGISTRY.impl.h

Jimara-StateMachines_BuildLibrary : $(STATE_MACHINES_BUILD_FILE)

$(STATE_MACHINES_BUILD_FILE) : $(STATE_MACHINES_INTERMEDIATE_FILES)
	mkdir -p $(ENGINE_BUILD_DIR) && ar rvs $(STATE_MACHINES_BUILD_FILE) $?

-include $(STATE_MACHINES_DEP_FILES)

$(STATE_MACHINES_INTERMEDIATE_DIR)/Navigation/NavMesh/NavMesh.o : $(STATE_MACHINES_SOURCE_DIR)/Navigation/NavMesh/NavMesh.cpp
	mkdir -p $(dir $@) && $(CXX) $(filter-out -O2,$(COMPILER_FLAGS)) -O1 $(BUILD_CONFIG_DEFINES) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@

$(STATE_MACHINES_INTERMEDIATE_DIR)/%.o : $(STATE_MACHINES_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@

# Compile State machines Editor tools:
.PHONY : Jimara-StateMachines-Editor_ImplementTypeRegistrator Jimara-StateMachines-Editor_BuildLibrary

Jimara-StateMachines-Editor :
	$(MAKE) Jimara-StateMachines-Editor_ImplementTypeRegistrator && \
	$(MAKE) Jimara-StateMachines-Editor_BuildLibrary -j $(NUM_PROCESSOR_CORES)

Jimara-StateMachines-Editor_ImplementTypeRegistrator : $(STATE_MACHINES_EDITOR_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(STATE_MACHINES_EDITOR_SOURCE_DIR) "Jimara::StateMachinesEditor_TypeRegistry" $(STATE_MACHINES_EDITOR_SOURCE_DIR)/__Generated__/JIMARA_STATE_MACHINES_EDITOR_TYPE_REGISTRY.impl.h

Jimara-StateMachines-Editor_BuildLibrary : $(STATE_MACHINES_EDITOR_BUILD_FILE)

$(STATE_MACHINES_EDITOR_BUILD_FILE) : $(STATE_MACHINES_EDITOR_INTERMEDIATE_FILES)
	mkdir -p $(ENGINE_BUILD_DIR) && ar rvs $(STATE_MACHINES_EDITOR_BUILD_FILE) $?

-include $(STATE_MACHINES_EDITOR_DEP_FILES)

$(STATE_MACHINES_EDITOR_INTERMEDIATE_DIR)/%.o : $(STATE_MACHINES_EDITOR_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) $(EDITOR_ADDITIONAL_INCLUDES) -c $< -o $@


# Compile Sample game .so:
.PHONY : Jimara-SampleGame_ImplementTypeRegistrator Jimara-SampleGame_BuildStaticObject

Jimara-SampleGame :
	$(MAKE) Jimara-Extensions && \
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)" "$(SAMPLE_GAME_SOURCE_DIR)" -id "$(SAMPLE_GAME_INTERMEDIATE_DIR)/__Generated_Shaders__" -o "$(EDITOR_BUILD_DIR)/Game/Shaders" && \
	$(MAKE) Jimara-SampleGame_ImplementTypeRegistrator && \
	$(MAKE) Jimara-SampleGame_BuildStaticObject -j $(NUM_PROCESSOR_CORES)

Jimara-SampleGame_ImplementTypeRegistrator : $(SAMPLE_GAME_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(SAMPLE_GAME_SOURCE_DIR) "Jimara::SampleGame::SampleGame_TypeRegistry" $(SAMPLE_GAME_SOURCE_DIR)/__Generated__/JIMARA_SAMPLE_GAME_TYPE_REGISTRY.impl.h

Jimara-SampleGame_BuildStaticObject : $(SAMPLE_GAME_INTERMEDIATE_FILES)
	mkdir -p $(EDITOR_BUILD_DIR)/Game && $(CXX) -dynamiclib -o $(SAMPLE_GAME_BUILD_FILE) $(COMPILER_FLAGS) $(SAMPLE_GAME_INTERMEDIATE_FILES) \
		$(SAMPLE_GAME_LINK_INPUTS) \
		-Wl,-force_load,$(GENERIC_INPUTS_BUILD_FILE) \
		-Wl,-force_load,$(STATE_MACHINES_BUILD_FILE) \
		-Wl,-force_load,$(STATE_MACHINES_EDITOR_BUILD_FILE) \
		$(ENGINE_THIRD_PARTY_LINKS)

-include $(SAMPLE_GAME_DEP_FILES)

$(SAMPLE_GAME_INTERMEDIATE_DIR)/%.o : $(SAMPLE_GAME_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && $(CXX) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# Clear build:
clean:
	rm -rf $(BUILD_DIR)/Intermediate ; \
	rm -rf $(ENGINE_BUILD_DIR) ; \
	rm -rf $(EDITOR_BUILD_DIR) ; \
	rm -rf $(TEST_BUILD_DIR) ;

# [Build and] run editor executable:
editor:
	$(MAKE) Jimara-Editor && cd $(EDITOR_BUILD_DIR) ; ./$(EDITOR_EXECUTABLE) $(EDITOR_CMD_ARGS) ; cd -

# Build and run a minimal native window smoke test:
WindowSmoke:
	$(MAKE) Jimara_BuildBinary -j $(NUM_PROCESSOR_CORES) && \
	$(CXX) -o $(WINDOW_TEST_BUILD) $(COMPILER_FLAGS) $(BUILD_CONFIG_DEFINES) $(WINDOW_TEST_SOURCE) -I$(SOURCE_DIR) $(ENGINE_THIRD_PARTY_INCLUDES) \
		$(ENGINE_BUILD_FILE) $(ENGINE_THIRD_PARTY_LINKS) && \
	./$(WINDOW_TEST_BUILD)

WindowTest: WindowSmoke

# [Build and] run test executable:
test:
	$(MAKE) Jimara-Test && cd $(TEST_BUILD_DIR) ; ./$(TEST_EXECUTABLE) $(TEST_CMD_ARGS) ; cd -
