# Third party include directories:
STB_INCLUDE_PATH = ../../Jimara-ThirdParty/stb
TERMCOLOR_INCLUDE_PATH = ../../Jimara-ThirdParty/termcolor/include
TINY_OBJ_INCLUDE_PATH = ../../Jimara-ThirdParty/tinyobjloader
X11_INCLUDE_PATH = -I/opt/X11/include -L/usr/X11/lib/

# Includes and links:
ENGINE_THIRD_PARTY_INCLUDES = -I$(STB_INCLUDE_PATH) -I$(TERMCOLOR_INCLUDE_PATH) -I$(TINY_OBJ_INCLUDE_PATH) $(X11_INCLUDE_PATH)
ENGINE_THIRD_PARTY_LINKS = -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi -lX11-xcb -lpthread
TEST_THIRD_PARTY_LINKS = -lgtest -lpthread

# Compiler flags:
COMPILER_FLAGS = -std=c++17 -O2 -MMD -MF -MF

# Base source folder and output:
SOURCE_DIR = ../../__Source__
BUILD_DIR = ../../__BUILD__

# Engine source files and output:
ENGINE_SOURCE_DIR = $(SOURCE_DIR)/Jimara
ENGINE_SOURCE_FILES = $(shell find $(ENGINE_SOURCE_DIR) -name '*.cpp')
ENGINE_BUILD_DIR = $(BUILD_DIR)/Jimara
ENGINE_INTERMEDIATE_DIR = $(ENGINE_BUILD_DIR)/Intermediate
ENGINE_INTERMEDIATE_FILES = $(subst $(ENGINE_SOURCE_DIR), $(ENGINE_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(ENGINE_SOURCE_FILES)))
ENGINE_BUILD_FILE = $(ENGINE_BUILD_DIR)/Jimara.a

# Engine unit test source files:
TEST_SOURCE_DIR = $(SOURCE_DIR)/Jimara-Tests
TEST_SOURCE_FILES = $(shell find $(TEST_SOURCE_DIR) -name '*.cpp')
TEST_BUILD_DIR = $(BUILD_DIR)/Jimara-Tests
TEST_INTERMEDIATE_DIR = $(TEST_BUILD_DIR)/Intermediate
TEST_INTERMEDIATE_FILES = $(subst $(TEST_SOURCE_DIR), $(TEST_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(TEST_SOURCE_FILES)))
TEST_EXECUTABLE = Jimara-Test
TEST_BUILD_FILE = $(TEST_BUILD_DIR)/$(TEST_EXECUTABLE)

# Compile Jimara-Test executable:
Jimara-Test : $(ENGINE_INTERMEDIATE_FILES) $(TEST_INTERMEDIATE_FILES)
	make Jimara; g++ -o $(TEST_BUILD_FILE) $(COMPILER_FLAGS) TestMain.cpp $^ $(TEST_THIRD_PARTY_LINKS) $(ENGINE_THIRD_PARTY_LINKS) \
	&& python3 ../../jimara_compile_shaders.py ../../__Source__ $(TEST_BUILD_DIR)/Shaders \
	&& ln -sfn ../../Jimara-BuiltInAssets $(TEST_BUILD_DIR)/Assets

$(TEST_INTERMEDIATE_DIR)/%.o : $(TEST_SOURCE_DIR)/%.cpp
	mkdir -p $(patsubst %.o, %/, $@) && g++ $(COMPILER_FLAGS) -I$(ENGINE_SOURCE_DIR) -c $< -o $@

# Compile Jimara engine library:
Jimara : $(ENGINE_INTERMEDIATE_FILES)
	ar rvs $(ENGINE_BUILD_FILE) $^

$(ENGINE_INTERMEDIATE_DIR)/%.o : $(ENGINE_SOURCE_DIR)/%.cpp
	mkdir -p $(patsubst %.o, %/, $@) && g++ $(COMPILER_FLAGS) $(ENGINE_THIRD_PARTY_INCLUDES) -c $< -o $@

# Additional functions:
.PHONY : clean test

# Clear build:
clean:
	rm -rf $(ENGINE_BUILD_DIR) && rm -rf $(TEST_BUILD_DIR)

# [Build and] run test executable:
test:
	make Jimara-Test && cd $(TEST_BUILD_DIR) ; ./$(TEST_EXECUTABLE) ; cd ../../Project/Linux
