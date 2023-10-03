
JIMARA_REPO ?= /home/dav23r/Jimara/
GAME_SOURCE_DIR ?= ./src
GAME_PROJECT_NAME = sample_game
GAME_NAMESPACE = sample_game::
GAME_BUILD_DIR = ./build-editor
GAME_INTERMEDIATE_DIR = ./build-intermediate

include $(JIMARA_REPO)/Project/Linux/EngineThirdParties

CXXFLAGS = -std=c++17 -O2 -MMD -MP -g -fPIC -rdynamic

GAME_SOURCE_FILES = $(shell find $(GAME_SOURCE_DIR) -name '*.cpp')
GAME_CPU_INTERMEDIATE_DIR = $(GAME_INTERMEDIATE_DIR)/cpu/
GAME_GPU_INTERMEDIATE_DIR = $(GAME_INTERMEDIATE_DIR)/gpu/

GAME_CPU_INTERMEDIATE_FILES = $(subst $(GAME_SOURCE_DIR), $(GAME_CPU_INTERMEDIATE_DIR), $(patsubst %.cpp, %.o, $(GAME_SOURCE_FILES)))
GAME_CPU_DEP_FILES = $(patsubst %.o, %.d, $(GAME_CPU_INTERMEDIATE_FILES))

GAME_STATIC_OBJ = $(GAME_BUILD_DIR)/Game/$(GAME_PROJECT_NAME).so

.PHONY: build-and-run build build-type-registrator build-static-obj
build:
	python "$(JIMARA_REPO)/__Scripts__/jimara_build_shaders.py" "$(JIMARA_REPO)/__Source__/Jimara" "$(GAME_SOURCE_DIR)" -id "$(GAME_GPU_INTERMEDIATE_DIR)" -o "$(GAME_BUILD_DIR)/Game/Shaders"
	make build-type-registrator
	make -j $(shell getconf _NPROCESSORS_ONLN) build-static-obj

.DEFAULT_GOAL : build-and-run
build-and-run: build
	cd $(GAME_BUILD_DIR)
	-mkdir -p Assets
	$(JIMARA_REPO)/__BUILD__/Linux/Jimara-Editor/Jimara-Editor
	cd -

build-type-registrator: $(GAME_SOURCE_FILES)
	python "$(JIMARA_REPO)/__Scripts__/jimara_implement_type_registrator.py" "$(GAME_SOURCE_DIR)" "$(GAME_NAMESPACE)$(GAME_PROJECT_NAME)_TypeRegistry" "$(GAME_SOURCE_DIR)/__Generated__/TypeRegistry.impl.h"

build-static-obj: $(GAME_CPU_INTERMEDIATE_FILES)
	mkdir -p "$(dir GAME_STATIC_OBJ)"
	g++ -shared -o "$(GAME_STATIC_OBJ)" $(CXXFLAGS) $(GAME_CPU_INTERMEDIATE_FILES) "$(JIMARA_REPO)"/__BUILD__/Linux/Jimara/Jimara.a

-include $(GAME_CPU_DEP_FILES)

$(GAME_CPU_INTERMEDIATE_DIR)/%.o: $(GAME_SOURCE_DIR)/%.cpp
	mkdir -p $(dir $@)
	g++ $(CXXFLAGS) -I"$(JIMARA_REPO)/__Source__" -c $< -o $@

