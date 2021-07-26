# include common dependencies
ENGINE_ROOT_DIR = ../../
include EngineThirdParties

# Compiler flags:
COMPILER_FLAGS = -std=c++17 -O2 -MMD -MP
# COMPILER_FLAGS += -DJIMARA_PLATFORM_SUPPORTS_WAYLAND

# Base source folder and output:
SOURCE_DIR = ../../__Source__
BUILD_DIR = ../../__BUILD__

# Engine source files and output:
ENGINE_SOURCE_DIR = $(SOURCE_DIR)/Jimara
ENGINE_SOURCE_FILES = $(shell find $(ENGINE_SOURCE_DIR) -name '*.cpp')
ENGINE_BUILD_DIR = $(BUILD_DIR)/Jimara
ENGINE_INTERMEDIATE_DIR = $(ENGINE_BUILD_DIR)/Intermediate
ENGINE_INTERMEDIATE_FILES = $(subst $(ENGINE_SOURCE_DIR), $(ENGINE_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(ENGINE_SOURCE_FILES)))
ENGINE_DEP_FILES = $(patsubst %.o, %.d, $(ENGINE_INTERMEDIATE_FILES)))
ENGINE_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara.a
ENGINE_LIGHT_DEFINITIONS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jld')
ENGINE_LIGHTING_MODELS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jlm')
ENGINE_LIT_SHADERS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jls')
ENGINE_LIGHT_ID_HEADER = $(ENGINE_SOURCE_DIR)/__Generated__/JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.h

# Engine unit test source files:
TEST_SOURCE_DIR = $(SOURCE_DIR)/Jimara-Tests
TEST_SOURCE_FILES = $(shell find $(TEST_SOURCE_DIR) -name '*.cpp')
TEST_BUILD_DIR = $(BUILD_DIR)/Jimara-Tests
TEST_INTERMEDIATE_DIR = $(TEST_BUILD_DIR)/Intermediate
TEST_INTERMEDIATE_FILES = $(subst $(TEST_SOURCE_DIR), $(TEST_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(TEST_SOURCE_FILES)))
TEST_DEP_FILES = $(patsubst %.o, %.d, $(TEST_INTERMEDIATE_FILES)))
TEST_EXECUTABLE = Jimara-Test
TEST_BUILD_FILE = $(TEST_BUILD_DIR)/$(TEST_EXECUTABLE)
TEST_LIGHT_DEFINITIONS = $(shell find $(TEST_SOURCE_DIR) -name '*.jld')
TEST_LIGHTING_MODELS = $(shell find $(TEST_SOURCE_DIR) -name '*.jlm')
TEST_LIT_SHADERS = $(shell find $(TEST_SOURCE_DIR) -name '*.jls')
TEST_LIGHT_ID_HEADER = $(TEST_SOURCE_DIR)/__Generated__/JIMARA_TEST_LIGHT_IDENTIFIERS.h

# Processor info:
NUM_PROCESSOR_CORES = $(shell getconf _NPROCESSORS_ONLN)

.PHONY : default Jimara Jimara-test
default : Jimara


# Compile Jimara engine library:
.PHONY : Jimara_BuildShaders Jimara_ImplementTypeRegistrator Jimara_BuildLibrary

Jimara : BuildPhysX_SDK
	make Jimara_BuildShaders && make Jimara_ImplementTypeRegistrator && make Jimara_BuildBinary -j $(NUM_PROCESSOR_CORES)

Jimara_BuildShaders : $(ENGINE_LIGHT_ID_HEADER)
Jimara_BuildBinary : $(ENGINE_BUILD_FILE)

Jimara_ImplementTypeRegistrator : $(ENGINE_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(ENGINE_SOURCE_DIR) "Jimara::BuiltInTypeRegistrator" $(ENGINE_SOURCE_DIR)/__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h

$(ENGINE_LIGHT_ID_HEADER) : $(ENGINE_LIGHT_DEFINITIONS) $(ENGINE_LIGHTING_MODELS) $(ENGINE_LIT_SHADERS)
	python3 ../../__Scripts__/jimara_build_shaders.py $(ENGINE_SOURCE_DIR) $(ENGINE_INTERMEDIATE_DIR)/__Generated_Shaders__ $(ENGINE_BUILD_DIR)/Shaders $(ENGINE_LIGHT_ID_HEADER)

$(ENGINE_BUILD_FILE) : $(ENGINE_INTERMEDIATE_FILES)
	ar rvs $(ENGINE_BUILD_FILE) $?

-include $(ENGINE_DEP_FILES)

$(ENGINE_INTERMEDIATE_DIR)/%.o : $(ENGINE_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# PhysX sdk compilation:
PHYSX_PROJECT_GENERATOR = $(PHYSX_MODULE_PATH)/physx/generate_projects.sh
PHYSX_PROJECT_DIR = $(PHYSX_MODULE_PATH)/physx/compiler/linux-$(PHYSX_CONFIG)
PHYSX_MAKEFILE = $(PHYSX_PROJECT_DIR)/Makefile
.PHONY : BuildPhysX_SDK
BuildPhysX_SDK : $(PHYSX_MAKEFILE)
	cd $(PHYSX_PROJECT_DIR) && make ; cd -
$(PHYSX_MAKEFILE) : $(PHYSX_PROJECT_GENERATOR)
	./$(PHYSX_PROJECT_GENERATOR) linux


# Compile Jimara-Test executable:
.PHONY : Jimara-test_BuildShaders Jimara-test_BuildExecutable

Jimara-test :
	make Jimara && make Jimara-test_BuildShaders && make Jimara-test_BuildExecutable -j $(NUM_PROCESSOR_CORES)
	python3 ../../jimara_compile_shaders.py ../../__Source__ $(TEST_BUILD_DIR)/Shaders
	ln -sfn ../../Jimara-BuiltInAssets $(TEST_BUILD_DIR)/Assets

Jimara-test_BuildShaders : $(TEST_LIGHT_ID_HEADER)

Jimara-test_BuildExecutable : $(TEST_BUILD_FILE)

$(TEST_LIGHT_ID_HEADER) : $(ENGINE_LIGHT_DEFINITIONS) $(ENGINE_LIGHTING_MODELS) $(ENGINE_LIT_SHADERS) $(TEST_LIGHT_DEFINITIONS) $(TEST_LIGHTING_MODELS) $(TEST_LIT_SHADERS)
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)|$(TEST_SOURCE_DIR)" $(TEST_INTERMEDIATE_DIR)/__Generated_Shaders__ $(TEST_BUILD_DIR)/Shaders $(TEST_LIGHT_ID_HEADER) 

$(TEST_BUILD_FILE) : $(ENGINE_BUILD_FILE) $(TEST_INTERMEDIATE_FILES)
	g++ -o $(TEST_BUILD_FILE) $(COMPILER_FLAGS) TestMain.cpp $(TEST_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE) $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS)

-include $(TEST_DEP_FILES)

$(TEST_INTERMEDIATE_DIR)/%.o : $(TEST_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) -I$(ENGINE_SOURCE_DIR) $(PHYSX_INCLUDE_PATH) -c $< -o $@


# Additional functions:
.PHONY : clean test

# Clear build:
clean:
	rm -rf $(ENGINE_BUILD_DIR) && rm -rf $(TEST_BUILD_DIR) && rm -f $(ENGINE_LIGHT_ID_HEADER) && rm -f $(TEST_LIGHT_ID_HEADER)

# [Build and] run test executable:
test:
	make Jimara-test && cd $(TEST_BUILD_DIR) ; ./$(TEST_EXECUTABLE) $(TEST_CMD_ARGS) ; cd -
