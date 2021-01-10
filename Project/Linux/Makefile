# Include paths
JIMARA_INCLUDE_PATH = ../../__Source__
STB_INCLUDE_PATH = ../../Jimara-ThirdParty/stb
TINY_OBJ_INCLUDE_PATH = ../../Jimara-ThirdParty/tinyobjloader
X11_INCLUDE_PATH = -I/opt/X11/include -L/usr/X11/lib/

# Compiler flags
CFLAGS = -std=c++17 -O2 -I$(JIMARA_INCLUDE_PATH) -I$(STB_INCLUDE_PATH) -I$(TINY_OBJ_INCLUDE_PATH) $(X11_INCLUDE_PATH)

# Linked libraries
LDFLAGS = -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi -lX11-xcb -lgtest -lpthread

# Engine source directory and files
SOURCEDIR = $(JIMARA_INCLUDE_PATH)
SRCS = $(shell find $(SOURCEDIR) -name '*.cpp')

# Engine unit test source directory and files
TESTDIR = ../../__Tests__
TEST_SRCS = $(shell find $(TESTDIR) -name '*.cpp')

.PHONY: test clean clean_test

# Compiles Executable that runs all unit tests for the engine
JimaraTest:
	g++ $(CFLAGS) TestMain.cpp -o ../../__BUILD__/JimaraTest $(SRCS) $(TEST_SRCS) $(LDFLAGS)

# Compiles and runs test executable
test: JimaraTest
	./../../__BUILD__/JimaraTest

# Removes test executable
clean:
	rm -f ../../__BUILD__/JimaraTest

# Removes old test executable, (re)compiles tests and runs the new file
clean_test: clean test
