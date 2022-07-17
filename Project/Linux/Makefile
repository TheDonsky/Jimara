# Available commands:
.PHONY : default Jimara-All Jimara Jimara-Editor editor Jimara-Test test Jimara-SampleGame clean
default : Jimara-All
Jimara-All:
	make Jimara && make Jimara-Editor && make Jimara-Test && make Jimara-SampleGame

# include common dependencies
ENGINE_ROOT_DIR = ../../
include EngineThirdParties

# Compiler flags:
COMPILER_FLAGS = -std=c++17 -O2 -MMD -MP -g -fPIC -rdynamic

# Base source folder and output:
SOURCE_DIR = ../../__Source__
BUILD_DIR = ../../__BUILD__/Linux

# Processor info:
NUM_PROCESSOR_CORES = $(shell getconf _NPROCESSORS_ONLN)

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
EDITOR_BUILD_DIR = $(BUILD_DIR)/Jimara-Editor
EDITOR_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-Editor
EDITOR_INTERMEDIATE_FILES = $(subst $(EDITOR_SOURCE_DIR), $(EDITOR_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(EDITOR_SOURCE_FILES)))
EDITOR_DEP_FILES = $(patsubst %.o, %.d, $(EDITOR_INTERMEDIATE_FILES))
EDITOR_EXECUTABLE = Jimara-Editor
EDITOR_BUILD_FILE = $(EDITOR_BUILD_DIR)/$(EDITOR_EXECUTABLE)
EDITOR_ADDITIONAL_INCLUDES = \
	-I$(ENGINE_SOURCE_DIR) \
	-I$(ENGINE_THIRD_PARTY_INCLUDES) \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/imgui \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/Font-Awesome \
	-I$(ENGINE_ROOT_DIR)/Jimara-ThirdParty/FontHeaders

# Engine unit test source files:
TEST_SOURCE_DIR = $(SOURCE_DIR)/Jimara-Tests
TEST_SOURCE_FILES = $(shell find $(TEST_SOURCE_DIR) -name '*.cpp')
TEST_BUILD_DIR = $(BUILD_DIR)/Jimara-Tests
TEST_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-Tests
TEST_INTERMEDIATE_FILES = $(subst $(TEST_SOURCE_DIR), $(TEST_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(TEST_SOURCE_FILES)))
TEST_DEP_FILES = $(patsubst %.o, %.d, $(TEST_INTERMEDIATE_FILES))
TEST_EXECUTABLE = Jimara-Test
TEST_BUILD_FILE = $(TEST_BUILD_DIR)/$(TEST_EXECUTABLE)
TEST_LIGHT_DEFINITIONS = $(shell find $(TEST_SOURCE_DIR) -name '*.jld')
TEST_LIGHTING_MODELS = $(shell find $(TEST_SOURCE_DIR) -name '*.jlm')
TEST_LIT_SHADERS = $(shell find $(TEST_SOURCE_DIR) -name '*.jls')

# Sample game source files and output:
SAMPLE_GAME_SOURCE_DIR = $(SOURCE_DIR)/Jimara-SampleGame
SAMPLE_GAME_SOURCE_FILES = $(shell find $(SAMPLE_GAME_SOURCE_DIR) -name '*.cpp')
SAMPLE_GAME_INTERMEDIATE_DIR = $(BUILD_DIR)/Intermediate/Jimara-SampleGame
SAMPLE_GAME_INTERMEDIATE_FILES = $(subst $(SAMPLE_GAME_SOURCE_DIR), $(SAMPLE_GAME_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(SAMPLE_GAME_SOURCE_FILES)))
SAMPLE_GAME_DEP_FILES = $(patsubst %.o, %.d, $(SAMPLE_GAME_INTERMEDIATE_FILES))
SAMPLE_GAME_BUILD_FILE = $(EDITOR_BUILD_DIR)/Game/SampleGame.so


# Compile Jimara engine library:
.PHONY : Jimara_ImplementTypeRegistrator Jimara_BuildLibrary

Jimara : BuildPhysX_SDK
	make Jimara_ImplementTypeRegistrator && make Jimara_BuildBinary -j $(NUM_PROCESSOR_CORES)

Jimara_BuildBinary : $(ENGINE_BUILD_FILE)

Jimara_ImplementTypeRegistrator : $(ENGINE_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(ENGINE_SOURCE_DIR) "Jimara::BuiltInTypeRegistrator" $(ENGINE_SOURCE_DIR)/__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h

$(ENGINE_BUILD_FILE) : $(ENGINE_INTERMEDIATE_FILES)
	mkdir -p $(ENGINE_BUILD_DIR) && ar rvs $(ENGINE_BUILD_FILE) $?

-include $(ENGINE_DEP_FILES)

$(ENGINE_INTERMEDIATE_DIR)/%.o : $(ENGINE_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# PhysX sdk compilation:
PHYSX_PROJECT_GENERATOR = $(PHYSX_MODULE_PATH)/physx/generate_projects.sh
PHYSX_PROJECT_DIR = $(PHYSX_MODULE_PATH)/physx/compiler/linux-$(PHYSX_CONFIG)
PHYSX_MAKEFILE = $(PHYSX_PROJECT_DIR)/Makefile
.PHONY : BuildPhysX_SDK
BuildPhysX_SDK : $(PHYSX_MAKEFILE)
	cd $(PHYSX_PROJECT_DIR) && make -j $(NUM_PROCESSOR_CORES) ; cd -
$(PHYSX_MAKEFILE) : $(PHYSX_PROJECT_GENERATOR)
	./$(PHYSX_PROJECT_GENERATOR) linux


# Compile Jimara-Editor executable:
Jimara-Editor: 
	make Jimara-SampleGame && make Jimara-Editor_ImplementTypeRegistrator && make Jimara-Editor_BuildExecutable -j $(NUM_PROCESSOR_CORES) && \
	ln -sfn ../../../Jimara-BuiltInAssets $(EDITOR_BUILD_DIR)/Assets

Jimara-Editor_BuildExecutable: $(EDITOR_INTERMEDIATE_FILES)
	g++ -o $(EDITOR_BUILD_FILE) $(COMPILER_FLAGS) $(EDITOR_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE) $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS)

-include $(EDITOR_DEP_FILES)

$(EDITOR_INTERMEDIATE_DIR)/%.o : $(EDITOR_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) $(EDITOR_ADDITIONAL_INCLUDES) -c $< -o $@

.PHONY : Jimara-Editor_ImplementTypeRegistrator

Jimara-Editor_ImplementTypeRegistrator : $(EDITOR_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(EDITOR_SOURCE_DIR) "Jimara::Editor::JimaraEditorTypeRegistry" $(EDITOR_SOURCE_DIR)/__Generated__/JIMARA_EDITOR_TYPE_REGISTRY.impl.h


# Compile Jimara-Test executable:
.PHONY : Jimara-Test_BuildExecutable

Jimara-Test :
	make Jimara && make Jimara-Test_BuildExecutable -j $(NUM_PROCESSOR_CORES) && \
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)" "$(TEST_SOURCE_DIR)" -id "$(TEST_INTERMEDIATE_DIR)/__Generated_Shaders__" -o "$(TEST_BUILD_DIR)/Shaders" && \
	g++ -std=c++17 -shared -o $(TEST_BUILD_DIR)/TestDLL_A.so -fPIC $(SOURCE_DIR)/Test-DLL-Files/TestDLL_A.cpp && \
	g++ -std=c++17 -shared -o $(TEST_BUILD_DIR)/TestDLL_B.so -fPIC $(SOURCE_DIR)/Test-DLL-Files/TestDLL_B.cpp -I$(SOURCE_DIR) $(ENGINE_BUILD_FILE) $(ENGINE_THIRD_PARTY_LINKS) && \
	ln -sfn ../../../Jimara-BuiltInAssets $(TEST_BUILD_DIR)/Assets

Jimara-Test_BuildExecutable : $(TEST_BUILD_FILE)

$(TEST_BUILD_FILE) : $(ENGINE_BUILD_FILE) $(TEST_INTERMEDIATE_FILES)
	mkdir -p $(TEST_BUILD_DIR) && g++ -o $(TEST_BUILD_FILE) $(COMPILER_FLAGS) TestMain.cpp $(TEST_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE) $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS)

-include $(TEST_DEP_FILES)

$(TEST_INTERMEDIATE_DIR)/%.o : $(TEST_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) -I$(ENGINE_SOURCE_DIR) -I$(JSON_INCLUDE_PATH) $(PHYSX_INCLUDE_PATH) -c $< -o $@


# Compile Sample game .so:
.PHONY : Jimara-SampleGame_ImplementTypeRegistrator Jimara-SampleGame_BuildStaticObject

Jimara-SampleGame :
	make Jimara && \
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)" "$(SAMPLE_GAME_SOURCE_DIR)" -id "$(SAMPLE_GAME_INTERMEDIATE_DIR)/__Generated_Shaders__" -o "$(EDITOR_BUILD_DIR)/Game/Shaders" && \
	make Jimara-SampleGame_ImplementTypeRegistrator && \
	make Jimara-SampleGame_BuildStaticObject -j $(NUM_PROCESSOR_CORES)

Jimara-SampleGame_ImplementTypeRegistrator : $(SAMPLE_GAME_SOURCE_FILES)
	python3 ../../__Scripts__/jimara_implement_type_registrator.py $(SAMPLE_GAME_SOURCE_DIR) "Jimara::SampleGame::SampleGame_TypeRegistry" $(SAMPLE_GAME_SOURCE_DIR)/__Generated__/JIMARA_SAMPLE_GAME_TYPE_REGISTRY.impl.h

Jimara-SampleGame_BuildStaticObject : $(SAMPLE_GAME_INTERMEDIATE_FILES)
	mkdir -p $(EDITOR_BUILD_DIR)/Game && g++ -shared -o $(SAMPLE_GAME_BUILD_FILE) $(COMPILER_FLAGS) $(SAMPLE_GAME_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE)

-include $(SAMPLE_GAME_DEP_FILES)

$(SAMPLE_GAME_INTERMEDIATE_DIR)/%.o : $(SAMPLE_GAME_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) -I$(SOURCE_DIR) -c $< -o $@


# Clear build:
clean:
	rm -rf $(BUILD_DIR)/Intermediate ; \
	rm -rf $(ENGINE_BUILD_DIR) ; \
	rm -rf $(EDITOR_BUILD_DIR) ; \
	rm -rf $(TEST_BUILD_DIR) ;

# [Build and] run editor executable:
editor:
	make Jimara-Editor && cd $(EDITOR_BUILD_DIR) ; ./$(EDITOR_EXECUTABLE) $(EDITOR_CMD_ARGS) ; cd -

# [Build and] run test executable:
test:
	make Jimara-Test && cd $(TEST_BUILD_DIR) ; ./$(TEST_EXECUTABLE) $(TEST_CMD_ARGS) ; cd -
