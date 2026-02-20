BUILD_DIR := build
TARGET_NAME := yabt

LUAJIT_LIBS := $(shell pkg-config --libs luajit)
LUAJIT_INCS := $(shell pkg-config --cflags luajit)

INCLUDE_DIRS := include

SRCS := src/main.cpp   \
	src/lua_engine.cpp \
	src/cmd/build.cpp

CFLAGS := -Wall -Wextra -Werror $(LUAJIT_INCS) -O2 -gdwarf-3 $(patsubst %, -I%, $(INCLUDE_DIRS))
CXXFLAGS := $(CFLAGS) -std=c++20
LDFLAGS := $(LUAJIT_LIBS)

OBJECTS := $(patsubst %.cpp, $(BUILD_DIR)/%.o, $(SRCS))

all: $(BUILD_DIR)/$(TARGET_NAME)
.PHONY: all

clean:
	rm -rf $(BUILD_DIR)
.PHONY: clean

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	g++ -c -o $@ $(CXXFLAGS) $^

$(BUILD_DIR)/$(TARGET_NAME): $(OBJECTS)
	@mkdir -p $(dir $@)
	g++ -o $@ $(CXXFLAGS) $^ $(LDFLAGS)
