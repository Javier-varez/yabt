# Yet another build tool (Yabt)

`Yabt` is a build tool inspired by the [Daedalean Build Tool (DBT)](https://github.com/daedaleanai/dbt). `Yabt` is responsible for:
- Managing project dependencies using a standardized dependency format and structure.
- Synchronizing dependencies from multiple origins like git source control.
- Maintaining a set of build rules that define how to build different kinds of build targets.
- Declaring build targets with a mostly-declarative syntax that is unambiguous and concise.
- Orchestrating execution of previously-built binaries, as well as enabling other flows like 
testing, code coverage collection and static analysis.

`Yabt` is opinionated. It enforces a specific repository layout, as well as ways to define build 
rules and build targets. 

Build rules and targets are written in [Lua](https://lua.org), which is a fantastic language to create 
sandboxed environments for build target definitions, while still providing a high
degree of flexibility when defining build rules.

`Yabt` delegates the actual build process to the [`Ninja`](https://ninja-build.org) build tool.

## Building

Depedencies:
- luajit 2.1
- A C++ compiler with support for `-std=c++20`. Tested with GCC 15 and clang 21.

While `Yabt` is self-hosting, it can be bootstrapped from a simple `makefile`. The `boostrap.sh`
script produces the resulting `yabt` binary in the root of this repository after bootstrapping.

```sh
./bootstrap.sh
```

The bootstrap process follows 3 stages:
- Stage 0: builds `Yabt` using the root `makefile`. The output will be under `./build/yabt`
- Stage 1: builds `Yabt` using the `yabt` tool generated in Stage 0. The output is generated under `BUILD_STAGE1`.
- Stage 2: verifies the output of Stage 1 by building `yabt` using the output of Stage 1. 
The output is generated under `BUILD_STAGE2`.

## Usage

This section contains multiple usage examples of the yabt tool:

### Fetch dependencies

```sh
yabt sync # fetches the dependencies of the top-module, including transitive dependencies.
```

Alternatively:

```sh
yabt sync --strict # fails if any dependency hash is empty.
```

### Build targets in a workspace

```sh
yabt build //target/spec/Target
```

Builds the given target(s) using the given Build Spec (see definition [below](#target-spec)).

### Clean the build outputs

```sh
yabt clean # Removes the BUILD directory at the root of the workspace.
```

### Get help!

```sh
yabt help # Generic help about the tool
```

```sh
yabt help sync # Get specific help output about the sync subcommand.
```

## Terminology

### `Yabt` module

A `Yabt` module is a directory containing any combination of build rules, build targets with their 
corresponding sources, and a module file indicating the dependencies of the module, in the form 
of references to other `Yabt` modules.

### Yabt module file

A `MODULE.lua` file located at the module top-level that defines:
- The name of the module. Other modules depending on this module must refer to it using this name.
- The version of the module file. This helps `Yabt` evolve the format of the module file in the future. 
At the time of this writing it is fixed to 1. <!-- NOTE: Update when module file format changes -->
- A list of dependencies of these modules, each containing:
    - The URL where the dependency is located.
    - A version string indicating the version of the dependency. The specific meaning is tied to the 
    type of the module (see below).
    - A hash string, which uniquely identifies all the content of the dependency. In contrast with
    the version string, which can liberally refer to a branch, without referring to a specific
    commit in it, the hash univocally identifies a single revision of the module. This field is optional.
    - A type string, identifying the type of resource located behind the URL. At this time `Yabt` only 
    supports 'Git'. It is optional if it can be identified from the URL (if the URL finishes with `.git`).
- A list of configuration flags, represented as key-value pairs of strings.

An example module file can be seen below:

```lua
return {
    name = 'my_module',
    version = 1,
    deps = {
        yabt_rules = {
            url = 'https://github.com/<path-to-yabt_rules>.git',
            version = 'origin/main', -- For git, this can be any kind of git reference (tags, branches, commit ids, etc)
            hash = '9a0e18b54a9fa3c769150c7ea0f61632d9070985', -- Optional. The specific hash of the reference listed in version
            type = 'Git', -- Optional. The type of dependency
        }
    }
    flags = {
        toolchain = 'gcc-toolchain',
    }
}
```

### Build rule

`Lua` code under `rules` that defines how to build a specific kind of target, as well as how to define it
as a build target in a build definition file.

### Build target

Definition of a specific output of the build process. Typically defines the output files generated 
by yabt, as well as the inputs it will use to generate it. For a static C++ library, this looks like:

```lua
local cc = require 'yabt_cc_rules.cc'

local utils = import 'project/utils' -- Imports file `src/project/utils/BUILD.lua` of the dependency named `project`

targets.MyLib = cc.Library:new {
    out = out('my_lib.a'),
    srcs = ins(
        'source1.cpp',
        'source2.cpp',
        'source3.cpp'
    ),
    deps = {
        utils.Lib, -- Dependencies of this library.
    }
}
```

### Workspace

The top-level module used to work on your project. Any module can act as a workspace, if checked-out 
independently. Use `yabt sync` to turn that module into a workspace. Afterwards, the `DEPS` directory 
will contain all dependencies of the top-level module.

### Target spec

A textual specification of the location of a target definition. All targets in a module are defined 
in `BUILD.lua` files under their `src/<module-name>` directory.

The target spec is made of 3 parts:
- `<module-name>`: Name of the module containing the build target.
- `<path-in-module>`: Path to the directory that defines the target (the directory containing the 
`BUILD.lua` file), relative to the `<module-path>/src/<module-name>` directory.
- `<target-name>`: The name of the target as defined in `BUILD.lua`.

`//<module-name>/<path-in-module>/<target-name>`

As an example, to build `Yabt` itself, you would use the following target spec:

`//yabt/Bin`

## Build rules vs build targets

### Build rules

Build rules are defined inside top-level `rules` directories. These directories are added to the `Lua`
`package.path` variable, such that `require` works for them. Code within build rules is intentionally 
unconstrained, which allows the engineers modifying rules to perform unbounded actions. The purpose 
of build rules is to create abstractions with simple interfaces such that targets can be defined 
in a naturally readable manner.

It is recommended, but not enforced, that build rules do not keep any global state, in order to avoid
non-explicit dependencies created across separate build targets.

### Build targets

Build targets are `BUILD.lua` files inside the `src/<project-name>` directory that define what is to be 
built, and using which rules. `Yabt` aggregates all `BUILD.lua` files and parses their contents to 
determine what targets are available.

A mechanism to reference read-only targets from other `BUILD.lua` files is provided by `Yabt`.
This allows a `BUILD.lua` file to create a one-way dependency to another one, enabling 
code reuse with the definition code libraries.

In contrast to build rules, build targets are heavily sandboxed, ensuring that:
- The actions in one `BUILD.lua` file do not alter the state of any rules defined elsewhere.
- `BUILD.lua` does not take arbitrary actions that could alter the filesystem or the state of the 
project. This is enforced by disabling built-in `Lua` libraries in `BUILD.lua` files.

## Motivation and origin

Originally, `dbt` used [Go](https://go.dev) instead. `Yabt` moves away from Go for two main reasons:
- Using `Go` requires the user to have a compatible go toolchain installed on their development workstation, 
even if they do not intend to use go for anything else in their projects. With `Lua` we can embed the
luajit compiler into the `yabt` executable, without requiring any additional dependencies.
- Using `Go` requires a separate compilation/execution step. While this is fine on its own, it makes 
incremental builds more complex and slow, as a single binary is created and executed.

It should be noted that `Go` also has some advantages with respect to `Lua`:
- `Go` is statically typed, which makes it easier to ensure the correctness of build rules, as well as
simplify the refactoring process of build rules, since the compiler helps you find all affected code.
`Yabt` mitigates this drawback by providing a framework to define tests for build rules, which helps to 
ensure that regressions can be found easily. Additionally, rules should eagerly check the arguments 
passed by build targets to detect type errors as soon as possible.
- Because `Go` is a compiled language, it is to be expected that its execution will be considerably 
faster than `Lua`, even when compiled just-in-time. However, because we can incrementally decide which 
build files have changed (and can map their dependency tree), the amount of work to be done will be 
smaller overall. If no build rules or targets change, there is no need to execute any of the `Lua` code 
before invoking `Ninja`.

