require 'yabt.cc.toolchain'.register_toolchain_as_default{
    name = 'GCC',
    ccompiler = 'gcc',
    cxxcompiler = 'g++',
    assembler = 'as',
    archiver = 'ar',
    linker = 'ld',
    cflags = { '-Wall', '-Wextra', '-Werror' },
    cxxflags = { '-Wall', '-Wextra', '-Werror' },
    asflags = {},
    ldflags = {},
    stddeps = {},
    ldscripts = {},
}
