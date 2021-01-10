JIMARA_INCLUDE_PATH = ../../__Source__
STB_INCLUDE_PATH = ../../Jimara-ThirdParty/stb
TINY_OBJ_INCLUDE_PATH = ../../Jimara-ThirdParty/tinyobjloader
X11_INCLUDE_PATH = -I/opt/X11/include -L/usr/X11/lib/
GTEST_INCLUDE_PATH = /usr/include/gtest

CFLAGS = -std=c++17 -O2 -I$(JIMARA_INCLUDE_PATH) -I$(STB_INCLUDE_PATH) -I$(TINY_OBJ_INCLUDE_PATH) $(X11_INCLUDE_PATH) -I$(GTEST_INCLUDE_PATH)
LDFLAGS = -lglfw -lvulkan -ldl -lX11 -lXxf86vm -lXrandr -lXi -lX11-xcb -lgtest -lpthread

SOURCEDIR = ../../__Source__
TESTDIR = ../../__Tests__

SRCS = $(shell find $(SOURCEDIR) -name '*.cpp')
TEST_SRCS = $(shell find $(TESTDIR) -name '*.cpp')

# cmake_minimum_required(VERSION 2.6)

JimaraTest:
	g++ $(CFLAGS) TestMain.cpp -o JimaraTest $(SRCS) $(TEST_SRCS) $(LDFLAGS)

.PHONY: test clean

test: JimaraTest
	./JimaraTest

clean:
	rm -f JimaraTest

cleanTest: clean test
