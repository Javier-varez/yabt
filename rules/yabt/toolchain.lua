local function detect_host_machine()
    local f = io.popen('uname -m')
    assert(f, 'unable to call `uname -m`')
    local arch = f:read('*l')
    f:close()
    if arch == nil then
        error("Unable to detect host machine")
    end
    return arch
end

local function detect_host_output_format()
    local f = io.popen('uname -s')
    assert(f, 'unable to call `uname -s`')
    local os_name = f:read('*l')
    f:close()
    if os_name == nil then
        error("Unable to detect host output format")
    elseif os_name == 'Darwin' then
        return 'macho'
    else
        return 'elf'
    end
end

require 'yabt_cc_rules.toolchain'.register_toolchain{
    name = 'clang',
    ccompiler = 'clang',
    cxxcompiler = 'clang++',
    assembler = 'llvm-as',
    archiver = 'llvm-ar',
    linker = 'clang++',
    raw_linker = 'lld',
    machine = detect_host_machine(),
    output_format = detect_host_output_format(),
    cflags = { '-Wall', '-Wextra', '-Werror', '-std=c17', '-O2', '-gdwarf-3' },
    cxxflags = { '-Wall', '-Wextra', '-Werror', '-std=c++20', '-O2', '-gdwarf-3', '-Wno-gnu-string-literal-operator-template', '-Wno-character-conversion' },
    asflags = {},
    ldflags = {},
    stddeps = {},
    ldscripts = {},
}
