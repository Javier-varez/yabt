THIS_FILE := $(lastword $(MAKEFILE_LIST))

BUILD_DIR := make_build
TARGET_NAME := yabt

LUAJIT_LIBS := $(shell pkg-config --libs luajit)
LUAJIT_INCS := $(shell pkg-config --cflags luajit)

INCLUDE_DIRS := include

SRCS := src/yabt/main.cpp                            \
    src/yabt/lua/lua_engine.cpp                      \
    src/yabt/lua/path_lib.cpp                        \
    src/yabt/lua/context_lib.cpp                     \
    src/yabt/lua/log_lib.cpp                         \
    src/yabt/log/log.cpp                             \
    src/yabt/cmd/build.cpp                           \
    src/yabt/cmd/clean.cpp                           \
    src/yabt/cmd/help.cpp                            \
    src/yabt/cmd/list.cpp                            \
    src/yabt/cmd/lsp.cpp                             \
    src/yabt/cmd/rules_test.cpp                      \
    src/yabt/cmd/run.cpp                             \
    src/yabt/cmd/sync.cpp                            \
    src/yabt/cmd/test.cpp                            \
    src/yabt/cli/cli_parser.cpp                      \
    src/yabt/cli/flag.cpp                            \
    src/yabt/process/process.cpp                     \
    src/yabt/module/module_file.cpp                  \
    src/yabt/utils/string.cpp                        \
    src/yabt/module/module.cpp                       \
    src/yabt/module/git_module.cpp                   \
    src/yabt/workspace/utils.cpp                     \
    src/yabt/ninja/ninja.cpp                         \
    src/yabt/build/args.cpp                          \
    src/yabt/build/build.cpp                         \
    src/yabt/embed/embed.cpp                         \
    src/yabt/embed/runtime.lua                       \
    src/yabt/embed/rules/yabt/core/utils.lua         \
    src/yabt/embed/lua_stubs/yabt/core/context.lua   \
    src/yabt/embed/lua_stubs/yabt/core/path.lua      \
    src/yabt/embed/lua_stubs/yabt/core/log.lua       \
    src/yabt/embed/lua_stubs/build_globals.lua

CFLAGS := -Wall -Wextra -Werror $(LUAJIT_INCS) -O2 -gdwarf-3 $(INCLUDE_DIRS:%=-I%) -Wno-gnu-string-literal-operator-template
CXXFLAGS := $(CFLAGS) -std=c++20
LDFLAGS := $(LUAJIT_LIBS)

CXX ?= g++

CPP_SRCS := $(filter %.cpp,$(SRCS))
LUA_SRCS := $(filter %.lua,$(SRCS))

OBJECTS := $(CPP_SRCS:%.cpp=$(BUILD_DIR)/%.o)
OBJECTS += $(LUA_SRCS:%.lua=$(BUILD_DIR)/%.lua.o)

DEPFILES := $(OBJECTS:%=%.d)

HOST_OS := $(shell uname -s)

# macOS uses different section names for mach-o binaries.
# Also prefixes all C symbols with an `_`.
ifeq ($(HOST_OS),Darwin)
ASM_SECTION := .section __TEXT,__const
ASM_SYM_PREFIX := _
ASM_GNU_STACK :=
else
ASM_SECTION := .section .rodata
ASM_SYM_PREFIX :=
ASM_GNU_STACK := .section .note.GNU-stack,"",@progbits
endif

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

$(BUILD_DIR)/%.lua.S: %.lua $(THIS_FILE)
	@mkdir -p $(dir $@)
	@symbol_name=$$(echo "$(patsubst src/%,%,$<)" | tr '/.' '_'); \
	{ \
	    echo '$(ASM_GNU_STACK)'; \
	    echo '$(ASM_SECTION)'; \
	    echo '.balign 16'; \
	    echo ".global $(ASM_SYM_PREFIX)_binary_$${symbol_name}_start"; \
	    echo "$(ASM_SYM_PREFIX)_binary_$${symbol_name}_start:"; \
	    printf '\t.incbin "%s"\n' "$(abspath $<)"; \
	    echo "$(ASM_SYM_PREFIX)_binary_$${symbol_name}_end:"; \
	    echo ".global $(ASM_SYM_PREFIX)_binary_$${symbol_name}_end"; \
	} > $@

$(BUILD_DIR)/%.lua.o: $(BUILD_DIR)/%.lua.S $(THIS_FILE)
	$(CXX) -c -o $@ $<

-include $(DEPFILES)
