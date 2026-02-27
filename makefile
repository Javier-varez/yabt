BUILD_DIR := build
TARGET_NAME := yabt

LUAJIT_LIBS := $(shell pkg-config --libs luajit)
LUAJIT_INCS := $(shell pkg-config --cflags luajit)

INCLUDE_DIRS := include

SRCS := src/yabt/main.cpp            \
	src/yabt/lua/lua_engine.cpp      \
	src/yabt/log/log.cpp             \
	src/yabt/cmd/build.cpp           \
	src/yabt/cmd/help.cpp            \
	src/yabt/cmd/sync.cpp            \
	src/yabt/cmd/clean.cpp           \
	src/yabt/cli/cli_parser.cpp      \
	src/yabt/cli/flag.cpp            \
	src/yabt/process/process.cpp     \
	src/yabt/module/module_file.cpp  \
	src/yabt/utils/string.cpp        \
	src/yabt/module/module.cpp       \
	src/yabt/module/git_module.cpp   \
	src/yabt/workspace/utils.cpp     \
	src/yabt/ninja/ninja.cpp         \
	src/yabt/build/build.cpp

CFLAGS := -Wall -Wextra -Werror $(LUAJIT_INCS) -O2 -gdwarf-3 $(INCLUDE_DIRS:%=-I%)
CXXFLAGS := $(CFLAGS) -std=c++20
LDFLAGS := $(LUAJIT_LIBS)

CXX ?= g++

OBJECTS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPFILES := $(OBJECTS:%=%.d)

all: $(BUILD_DIR)/$(TARGET_NAME)
.PHONY: all

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c -o $@ $(CXXFLAGS) -MD -MF $@.d $<

$(BUILD_DIR)/$(TARGET_NAME): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) -o $@ $(CXXFLAGS) $^ $(LDFLAGS)

-include $(DEPFILES)
