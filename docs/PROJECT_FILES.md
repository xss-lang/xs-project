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

sources {
  include("sources/**/*.xs")
  exclude("sources/tests/**")
}

test {
  include("tests/**/*.xs")
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
imports and diagnostics retain their file boundary.

`project(name, channel, version)` and at least one source include are required. An include may name a concrete
project-relative `.xs` file or use `*`, `**`, and `?` glob segments. Excludes are applied after all includes, duplicate
matches are removed, and the final registry is sorted deterministically with `main.xs` first. The resolved set must
contain exactly one case-sensitive `main.xs`; it is always the entry file. A pattern that matches no files is an error.

The DSL also provides:

- `set(name, value, ...)` for one or more versioned/compiler and user build values;
- `get(name)` to read a previously defined variable, with an error for an unknown name. Read-only project identity is
  available as `PROJECT`; ordinary variables include values such as `XS_VERSION`, `XS_BACKEND`, and `TARGET` when the
  script defines them;
- `getAll(name)` to read every value without joining a multi-value setting;
- `authors(...)` for project authors;
- `dependencies { addModule(...) }` for external module coordinates;
- `test { include(...); framework(...) }` for test discovery metadata;
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
`--warning`, `--werror`, and `--verbose` values are one-shot overrides applied after KTS evaluation; they never rewrite
the project script. The defaults are `warnings("medium")`, `werror(false)`, and `verbose(true)`. XSPROJ intentionally has
no persistent equivalent.

## Source registries

`sources` builds the exact source registry passed to the JVM-free compiler. Concrete paths and glob patterns may be
mixed in the same block:

```kotlin
sources {
  include("sources/main.xs")
  include("sources/app/**/*.xs")
  include("sources/platform/${OS.name.lowercase()}/**/*.xs")
  exclude("sources/app/generated/**")
  exclude("sources/**/*_test.xs")
}
```

The supported pattern operators are:

| Pattern | Meaning |
| --- | --- |
| `*` | zero or more characters inside one path segment |
| `?` | exactly one character inside one path segment |
| `**` | zero or more complete path segments |

Includes are evaluated relative to the project directory and must not escape it. Every include must resolve at least
one regular file. Excludes are evaluated after includes, duplicate paths are removed, and the resulting registry is
sorted deterministically. It is an error if exclusion removes the entry file or if the final registry does not contain
exactly one case-sensitive `main.xs`.

For a small project, listing every source is also valid:

```kotlin
sources {
  include("sources/main.xs")
  include("sources/math/vector.xs")
  include("sources/io/terminal.xs")
}
```

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
sources {
  include("sources/**/*.xs")
  exclude("sources/tests/**")
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
  include("tests/**/*.xs")
  framework("xs-test")
}

compiler {
  warnings("all")
  werror(true)
  verbose(false)
}
```

The split form is stateful. Across the final accumulated state, `project(...)` must be called exactly once and at least
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
