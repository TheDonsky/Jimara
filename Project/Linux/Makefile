# Third party include directories:
STB_INCLUDE_PATH = ../../Jimara-ThirdParty/stb
TERMCOLOR_INCLUDE_PATH = ../../Jimara-ThirdParty/termcolor/include
TINY_OBJ_INCLUDE_PATH = ../../Jimara-ThirdParty/tinyobjloader
SPIRV_REFLECT_INCLUDE_PATH = ../../Jimara-ThirdParty/SPIRV-Reflect
X11_INCLUDE_PATH = -I/opt/X11/include -L/usr/X11/lib/

# Includes and links:
ENGINE_THIRD_PARTY_INCLUDES = -I$(STB_INCLUDE_PATH) -I$(TERMCOLOR_INCLUDE_PATH) -I$(TINY_OBJ_INCLUDE_PATH) -I$(SPIRV_REFLECT_INCLUDE_PATH) $(X11_INCLUDE_PATH)
ENGINE_THIRD_PARTY_LINKS = -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi -lX11-xcb -lpthread
TEST_THIRD_PARTY_LINKS = -lgtest -lpthread

# Compiler flags:
COMPILER_FLAGS = -std=c++17 -O2 -MMD -MP

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
TEST_LIGHT_DEFINITIONS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jld')
TEST_LIGHTING_MODELS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jlm')
TEST_LIT_SHADERS = $(shell find $(ENGINE_SOURCE_DIR) -name '*.jls')
TEST_LIGHT_ID_HEADER = $(TEST_SOURCE_DIR)/__Generated__/JIMARA_TEST_LIGHT_IDENTIFIERS.h

.PHONY : default Jimara Jimara-test
default : Jimara


# Compile Jimara engine library:
Jimara : $(ENGINE_BUILD_FILE)

$(ENGINE_LIGHT_ID_HEADER) : $(ENGINE_LIGHT_DEFINITIONS) $(ENGINE_LIGHTING_MODELS) $(ENGINE_LIT_SHADERS)
	python3 ../../__Scripts__/jimara_build_shaders.py $(ENGINE_SOURCE_DIR) $(ENGINE_INTERMEDIATE_DIR)/__Generated_Shaders__ $(ENGINE_BUILD_DIR)/Shaders $(ENGINE_LIGHT_ID_HEADER)

$(ENGINE_BUILD_FILE) : $(ENGINE_LIGHT_ID_HEADER) $(ENGINE_INTERMEDIATE_FILES)
	ar rvs $(ENGINE_BUILD_FILE) $?

-include $(ENGINE_DEP_FILES)

$(ENGINE_INTERMEDIATE_DIR)/%.o : $(ENGINE_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@


# Compile Jimara-Test executable:
Jimara-test : $(TEST_BUILD_FILE)
	python3 ../../jimara_compile_shaders.py ../../__Source__ $(TEST_BUILD_DIR)/Shaders
	ln -sfn ../../Jimara-BuiltInAssets $(TEST_BUILD_DIR)/Assets

$(TEST_LIGHT_ID_HEADER) : $(ENGINE_LIGHT_DEFINITIONS) $(ENGINE_LIGHTING_MODELS) $(ENGINE_LIT_SHADERS) $(TEST_LIGHT_DEFINITIONS) $(TEST_LIGHTING_MODELS) $(TEST_LIT_SHADERS)
	python3 ../../__Scripts__/jimara_build_shaders.py "$(ENGINE_SOURCE_DIR)|$(TEST_SOURCE_DIR)" $(TEST_INTERMEDIATE_DIR)/__Generated_Shaders__ $(TEST_BUILD_DIR)/Shaders $(TEST_LIGHT_ID_HEADER) 

$(TEST_BUILD_FILE) : $(TEST_LIGHT_ID_HEADER) $(ENGINE_BUILD_FILE) $(TEST_INTERMEDIATE_FILES)
	g++ -o $(TEST_BUILD_FILE) $(COMPILER_FLAGS) TestMain.cpp $(TEST_INTERMEDIATE_FILES) $(ENGINE_BUILD_FILE) $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS)

-include $(TEST_DEP_FILES)

$(TEST_INTERMEDIATE_DIR)/%.o : $(TEST_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@) && g++ $(COMPILER_FLAGS) -I$(ENGINE_SOURCE_DIR) -c $< -o $@


# Additional functions:
.PHONY : clean test

# Clear build:
clean:
	rm -rf $(ENGINE_BUILD_DIR) && rm -rf $(TEST_BUILD_DIR) && rm -f $(ENGINE_LIGHT_ID_HEADER) && rm -f $(TEST_LIGHT_ID_HEADER)

# [Build and] run test executable:
test: Jimara-test
	cd $(TEST_BUILD_DIR) ; ./$(TEST_EXECUTABLE) ; cd -
