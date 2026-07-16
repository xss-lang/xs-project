<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Project files and packages

The modern X# project resolver is `/usr/bin/xs-project`. It requires JRE 25 or newer and an external `kotlin` command with
the Kotlin scripting runtime. The JRE and Kotlin runtime are not embedded. Project scripts are trusted build code, not
sandboxed input, and execution is delegated to the real Kotlin command; xs-project has no Kotlin interpreter.

A project may use one combined `xs.project.kts` file:

```kotlin
project("Example", "BETA", "0.1.0")

set("XS_VERSION", "0.1.7")
set("XS_BACKEND", "LLVM")
set(
  "TARGET",
  "x86_64-unknown-linux-gnu",
  "x86_64-unknown-linux-musl",
  "aarch64-unknown-linux-gnu",
  "armv7h-unknown-linux-gnueabihf",
  "riscv64gc-unknown-linux-gnu",
  "x86_64-apple-darwin",
  "aarch64-apple-darwin",
  "x86_64-pc-windows-msvc",
  "aarch64-pc-windows-msvc",
  "x86_64-unknown-freebsd",
  "aarch64-unknown-freebsd",
)

if (cfg(OS == LINUX) && cfg(ARCH == X86_64)) {
  set("NATIVE_TARGET", "x86_64-unknown-linux-gnu")
} else if (cfg(FAMILY == BSD)) {
  set("NATIVE_TARGET", "x86_64-unknown-freebsd")
} else {
  panic("unsupported build host")
}

authors(
  arrayOf("Leitwolf", "leitwolf@example.me"),
  arrayOf("Helmut", "helmut@example.me"),
)

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
  addModule("https://github.com/xss-lang/externalModules/::XML::0.1.0")
}

source {
  include("Sources")
  exclude("Sources/tests/**")
}

module {
  include("Modules")
}

test {
  include("Tests")
  exclude("Tests/fixtures/**")
  framework("xs-test")
}

compiler {
  warnings("all")
  werror(true)
  verbose(false)
}
```

Alternatively, configuration may be split between `xs.settings.kts` and `xs.build.kts`. Both files are required in split
mode, and `xs.project.kts` cannot coexist with them. Each file is evaluated separately by `kotlin`, so normal Kotlin
import and diagnostics retain their file boundary.

`project(name, channel, version)` and at least one source include are required. `source` is the canonical block name;
`sources` remains a compatibility alias. An include names a project-relative directory and recursively selects files
with the configured source extension. Include roots do not accept globs. Excludes may use `*`, `**`, and `?`; they are
applied after discovery. The default extension is `xs`, so the registry must contain exactly one case-sensitive
`main.xs`. `set("XS_EXTENSION", "xsharp")` instead selects `.xsharp` files and requires `main.xsharp`.

The DSL also provides:

- `set(name, value, ...)` for one or more versioned/compiler and user build values;
- `get(name)` to read a previously defined variable, with an error for an unknown name. Read-only project identity is
  available as `PROJECT`; ordinary variables include values such as `XS_VERSION`, `XS_BACKEND`, and `TARGET` when the
  script defines them;
- `getAll(name)` to read every value without joining a multi-value setting;
- `authors(...)` for project authors;
- `dependencies { addModule(...) }` for external module coordinates;
- `module { include(...) }` for the pool assigned by `xs.module.kts`;
- `test { include(...); exclude(...); framework(...) }` for recursive test discovery metadata;
- `compiler { warnings(...); werror(...); verbose(...) }` for diagnostic policy;
- `cfg(...)`, `OS`, `FAMILY`, and `ARCH` for ordinary Kotlin conditional configuration;
- `panic(...)` to reject the project configuration.

`PROJECT` is formatted as one stable identity string:

```kotlin
println(get("PROJECT"))
```

```text
Example, BETA, 0.1.0
```

The compiler policy is transferred with the resolved source registry to the JVM-free `xs` process. Command-line
`--warning`, `--werror`, `--verbose`, and `--xgc-enabled` values are one-shot overrides applied after KTS evaluation;
they never rewrite the project script. The defaults are `warnings("medium")`, `werror(false)`, `verbose(true)`, and
`set("XGC_ENABLED", false)`. XSPROJ intentionally has no persistent equivalent.

## Source registries

`source` defines one or more directory roots for the exact source registry passed to the JVM-free compiler:

```kotlin
source {
  include("Sources")
  include("Platform/${OS.name.lowercase()}")
  exclude("Sources/generated/**")
  exclude("Sources/**/*_test.xs")
}
```

The supported pattern operators are:

| Pattern | Meaning |
| --- | --- |
| `*` | zero or more characters inside one path segment |
| `?` | exactly one character inside one path segment |
| `**` | zero or more complete path segments |

Include roots are evaluated relative to the project directory, must exist as directories, and must not escape the
project. They are searched recursively. Excludes are evaluated after includes, duplicate paths are removed, and the
resulting registry is sorted deterministically. It is an error if exclusion removes the entry file or if the final
registry does not contain exactly one case-sensitive entry name for `XS_EXTENSION`.

## Module registries

X# source files do not contain a `module` declaration. Projects that need importable modules select a module source pool
in `xs.project.kts` and assign every selected file in a sibling `xs.module.kts` file:

```kotlin
// xs.project.kts
project("Example", "BETA", "0.1.0")

source {
  include("Sources")
}

module {
  include("Modules")
}
```

```kotlin
// xs.module.kts
module {
  name("MyModule")
  members {
    add("Modules/Example/*.xs")
  }

  submodule {
    name("util")
    add("Modules/Example/Utils/**/*.xs")
  }
}

module {
  name("FooModule")
  add("Modules/Foo/*.xs")
}
```

`add(...)` accepts a concrete source path or a glob. Direct `add` and `members` entries expose declarations as
`MyModule::<item>`; `submodule` above exposes them as `MyModule::util::<item>`. Every file selected by the project module
pool must be assigned exactly once. Missing/empty matches, duplicate assignments, unassigned module files, and overlap
between the ordinary source and module registries are configuration errors.

The project normally declares the module root with `module { include("Modules") }`. If it does not, the compiler
invocation must provide it explicitly with `xs build --module ./Modules`. The same explicit option enables a legacy
`.xsproj` build to consume the sibling `xs.module.kts`; `.xsproj` never enables module discovery implicitly.

`xs.module.kts` requires `xs-project`; `xs-compiler` does not execute Kotlin scripts. Argument-free `xs build`/`xs check`
ask `xs-project` for a resolved registry containing physical paths and logical module names. A direct `-file` invocation
does not load module project metadata.

For `.xs` sources, `PascalCase` directories and `snake_case.xs` file names are canonical but not mandatory.

## Split project scripts

There is no section-to-file ownership rule. `xs.settings.kts` is evaluated first, then `xs.build.kts` receives and may
extend the complete accumulated state. Either file may call any DSL function. The following is only one possible
arrangement, with identity and shared metadata in the first file:

```kotlin
// xs.settings.kts
project("Example", "BETA", "0.1.0")
set("XS_BACKEND", "LLVM")
authors(arrayOf("Leitwolf", "leitwolf@example.me"))

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
}
```

The second file then adds sources and compiler policy:

```kotlin
// xs.build.kts
source {
  include("Sources")
  exclude("Sources/tests/**")
}

set(
  "TARGET",
  "x86_64-unknown-linux-gnu",
  "x86_64-unknown-linux-musl",
  "aarch64-unknown-linux-gnu",
  "armv7h-unknown-linux-gnueabihf",
  "riscv64gc-unknown-linux-gnu",
  "x86_64-apple-darwin",
  "aarch64-apple-darwin",
  "x86_64-pc-windows-msvc",
  "aarch64-pc-windows-msvc",
  "x86_64-unknown-freebsd",
  "aarch64-unknown-freebsd",
)

test {
  include("Tests")
  framework("xs-test")
}

compiler {
  warnings("all")
  werror(true)
  verbose(false)
}
```

The split form is stateful. `xs.settings.kts` is evaluated first, `xs.build.kts` second, and an optional
`xs.module.kts` last. The module script receives the accumulated project state. Across the final state,
`project(...)` must be called exactly once and at least
one source include must exist; neither requirement is tied to a particular file. Normal Kotlin expressions, local
values, string interpolation, collections, loops, and conditionals remain available because these are real Kotlin
scripts rather than a simulated Kotlin parser.

BSD hosts are members of both the `BSD` and `UNIX` families. Consequently, `cfg(FAMILY == BSD)` and
`cfg(FAMILY == UNIX)` are both true on FreeBSD, OpenBSD, and NetBSD. Linux and macOS satisfy only `FAMILY == UNIX`;
they do not satisfy `FAMILY == BSD`.
Windows and ReactOS satisfy `FAMILY == WINDOWS`. ReactOS intentionally has no public `OS == REACTOS` DSL value and does
not satisfy `OS == WINDOWS`; family checks are the portable way to select both hosts.

Argument-free `xs build`, `xs check`, and `xs run` search the current directory and its parents through `xs-project`.
The resolver evaluates Kotlin, expands source metadata, and returns an exact source registry; it never parses or compiles
`.xs` contents. The JVM-free
`xs` process owns source parsing, semantic analysis, code generation, and artifacts. `XS_KOTLIN` may select the required
Kotlin command, while `XS_PROJECT_DRIVER` may select the resolver executable used by `xs`.

## Legacy XSPROJ

The C23 `.xsproj` parser API and `/usr/bin/xs-proj` parser/validator remain available, but the format is
feature-frozen. Dependency declarations are no longer part of XSPROJ. New programmable configuration and dependency
features are exclusive to the Kotlin project system. Legacy builds use `xs build -proj App.xsproj`; the `-proj` flag is
never used for Kotlin project files. XSPROJ is permanent legacy compatibility: it will not receive new features, it is
not used by compiler conformance/project build tests, and it will never be removed.

## Distribution packages

- `xs-compiler` installs `/usr/bin/xs` and does not require a JVM.
- `xs-project` installs `/usr/bin/xs-project` and requires JRE 25 or newer plus the `kotlin` scripting command.
- `xs-proj` installs the JVM-free `/usr/bin/xs-proj` manifest parser/validator.
- `xs-meta` is a metapackage depending on those packages and, when available, `xsfmt` and `xstidy`; it owns no compiler
  executable.
