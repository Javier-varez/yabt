BUILD_DIR := build
TARGET_NAME := yabt

LUAJIT_LIBS := $(shell pkg-config --libs luajit)
LUAJIT_INCS := $(shell pkg-config --cflags luajit)

INCLUDE_DIRS := include

SRCS := src/main.cpp            \
	src/lua/lua_engine.cpp      \
	src/log/log.cpp             \
	src/cmd/build.cpp           \
	src/cmd/help.cpp            \
	src/cmd/sync.cpp            \
	src/cli/cli_parser.cpp      \
	src/cli/flag.cpp            \
	src/process/process.cpp

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
