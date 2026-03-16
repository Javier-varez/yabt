THIS_FILE := $(lastword $(MAKEFILE_LIST))

BUILD_DIR := build
TARGET_NAME := yabt

LUAJIT_LIBS := $(shell pkg-config --libs luajit)
LUAJIT_INCS := $(shell pkg-config --cflags luajit)

INCLUDE_DIRS := include

SRCS := src/yabt/main.cpp                      \
    src/yabt/lua/lua_engine.cpp                \
    src/yabt/lua/path_lib.cpp                  \
    src/yabt/lua/context_lib.cpp               \
    src/yabt/lua/log_lib.cpp                   \
    src/yabt/log/log.cpp                       \
    src/yabt/cmd/build.cpp                     \
    src/yabt/cmd/help.cpp                      \
    src/yabt/cmd/sync.cpp                      \
    src/yabt/cmd/clean.cpp                     \
    src/yabt/cmd/list.cpp                      \
    src/yabt/cli/cli_parser.cpp                \
    src/yabt/cli/flag.cpp                      \
    src/yabt/process/process.cpp               \
    src/yabt/module/module_file.cpp            \
    src/yabt/utils/string.cpp                  \
    src/yabt/module/module.cpp                 \
    src/yabt/module/git_module.cpp             \
    src/yabt/workspace/utils.cpp               \
    src/yabt/ninja/ninja.cpp                   \
    src/yabt/build/build.cpp                   \
    src/yabt/embed/embed.cpp                   \
    src/yabt/embed/runtime.lua                 \
    src/yabt/embed/rules/yabt/core/utils.lua

CFLAGS := -Wall -Wextra -Werror $(LUAJIT_INCS) -O2 -gdwarf-3 $(INCLUDE_DIRS:%=-I%) -Wno-gnu-string-literal-operator-template
CXXFLAGS := $(CFLAGS) -std=c++20
LDFLAGS := $(LUAJIT_LIBS)

CXX ?= g++

CPP_SRCS := $(filter %.cpp,$(SRCS))
LUA_SRCS := $(filter %.lua,$(SRCS))

OBJECTS := $(CPP_SRCS:%.cpp=$(BUILD_DIR)/%.o)
OBJECTS += $(LUA_SRCS:%.lua=$(BUILD_DIR)/%.lua.o)

DEPFILES := $(OBJECTS:%=%.d)

TARGET_ARCH ?= $(shell uname -m)

all: $(BUILD_DIR)/$(TARGET_NAME)
.PHONY: all

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

$(BUILD_DIR)/%.o: %.cpp $(THIS_FILE)
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) -MD -MF $@.d $<

$(BUILD_DIR)/$(TARGET_NAME): $(OBJECTS) $(THIS_FILE)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(CXXFLAGS) $(OBJECTS) $(LDFLAGS)

$(BUILD_DIR)/%.lua.o: SRC_DIR := $(realpath $(dir $(THIS_FILE)))/src
$(BUILD_DIR)/%.lua.o: %.lua $(THIS_FILE)
	@mkdir -p $(dir $@)
	cd $(SRC_DIR) && ld -z noexecstack -m elf_$(TARGET_ARCH) -r -b binary -o $(abspath $@) $(patsubst $(SRC_DIR)/%, %, $(realpath $<))

-include $(DEPFILES)
