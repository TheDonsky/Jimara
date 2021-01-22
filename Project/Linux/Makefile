# Include paths
JIMARA_INCLUDE_PATH = ../../__Source__/Jimara
STB_INCLUDE_PATH = ../../Jimara-ThirdParty/stb
TERMCOLOR_INCLUDE_PATH = ../../Jimara-ThirdParty/termcolor/include
TINY_OBJ_INCLUDE_PATH = ../../Jimara-ThirdParty/tinyobjloader
X11_INCLUDE_PATH = -I/opt/X11/include -L/usr/X11/lib/

# Compiler flags
CFLAGS = -std=c++17 -O2 -I$(JIMARA_INCLUDE_PATH) -I$(STB_INCLUDE_PATH) -I$(TERMCOLOR_INCLUDE_PATH) -I$(TINY_OBJ_INCLUDE_PATH) $(X11_INCLUDE_PATH)

# Linked libraries
LDFLAGS = -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi -lX11-xcb -lgtest -lpthread

# Engine source directory and files
SOURCEDIR = $(JIMARA_INCLUDE_PATH)
SRCS = $(shell find $(SOURCEDIR) -name '*.cpp')

# Engine unit test source directory and files
TESTDIR = ../../__Source__/Jimara-Tests
TEST_SRCS = $(shell find $(TESTDIR) -name '*.cpp')

.PHONY: test clean clean_test

# Compiles Executable that runs all unit tests for the engine
JimaraTest:
	g++ $(CFLAGS) TestMain.cpp -o ../../__BUILD__/JimaraTest $(SRCS) $(TEST_SRCS) $(LDFLAGS) \
	&& python3 ../../jimara_compile_shaders.py ../../__Source__ ../../__BUILD__/Shaders

# Compiles and runs test executable
test: JimaraTest
	cd ../../__BUILD__ ; ./JimaraTest ; cd ../Project/Linux

# Removes test executable
clean:
	rm -f ../../__BUILD__/JimaraTest

# Removes old test executable, (re)compiles tests and runs the new file
clean_test: clean test
