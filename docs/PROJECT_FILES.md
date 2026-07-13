<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Project files and packages

The modern X# project resolver is `/usr/bin/xs-project`. It requires exactly JRE 25 and an external `kotlin` command with
the Kotlin scripting runtime. The JRE and Kotlin runtime are not embedded. Project scripts are trusted build code, not
sandboxed input, and execution is delegated to the real Kotlin command; xs-project has no Kotlin interpreter.

A project may use one combined `xs.project.kts` file:

```kotlin
project("Example", "BETA", "0.1.0")

set("XS_VERSION", "0.1.6")
set("XS_BACKEND", "LLVM")

if (cfg(OS == LINUX && ARCH == X86_64)) {
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

targets {
  target("x86_64-unknown-linux-gnu")
  target("aarch64-unknown-linux-gnu")
  target("x86_64-pc-windows-msvc")
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

- `set(name, value)` for versioned/compiler and user build variables;
- `authors(...)` for project authors;
- `dependencies { addModule(...) }` for external module coordinates;
- `targets { target(...) }` for requested target triples;
- `test { include(...); framework(...) }` for test discovery metadata;
- `compiler { warnings(...); werror(...); verbose(...) }` for diagnostic policy;
- `cfg(...)`, `OS`, `FAMILY`, and `ARCH` for ordinary Kotlin conditional configuration;
- `panic(...)` to reject the project configuration.

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

The settings file owns project identity and repository-wide metadata:

```kotlin
// xs.settings.kts
project("Example", "BETA", "0.1.0")
set("XS_BACKEND", "LLVM")
authors(arrayOf("Leitwolf", "leitwolf@example.me"))

dependencies {
  addModule("https://github.com/xss-lang/externalModules/::JSON::0.1.0")
}
```

The build file owns source selection and compiler policy:

```kotlin
// xs.build.kts
sources {
  include("sources/**/*.xs")
  exclude("sources/tests/**")
}

targets {
  target("x86_64-unknown-linux-gnu")
  target("aarch64-unknown-linux-gnu")
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

The split form is stateful: `xs.settings.kts` is evaluated first, its validated project state is transferred by
`xs-project`, and `xs.build.kts` extends that state. Normal Kotlin expressions, local values, string interpolation,
collections, loops, and conditionals remain available because these are real Kotlin scripts rather than a simulated
Kotlin parser.

BSD hosts are members of both the `BSD` and `UNIX` families. Consequently, `cfg(FAMILY == BSD)` and
`cfg(FAMILY == UNIX)` are both true on FreeBSD, OpenBSD, and NetBSD.

Argument-free `xs build`, `xs check`, and `xs run` search the current directory and its parents through `xs-project`.
The resolver evaluates Kotlin, expands source metadata, and returns an exact source registry; it never parses or compiles
`.xs` contents. The JVM-free
`xs` process owns source parsing, semantic analysis, code generation, and artifacts. `XS_KOTLIN` may select the required
Kotlin command, while `XS_PROJECT_DRIVER` may select the resolver executable used by `xs`.

## Legacy XSPROJ

The C23 `.xsproj` parser API and `/usr/bin/xs-proj` parser/validator remain available, but the format is
feature-frozen. Dependency declarations are no longer part of XSPROJ. New programmable configuration and dependency
features are exclusive to the Kotlin project system. Legacy builds use `xs build -proj App.xsproj`; the `-proj` flag is
never used for Kotlin project files.

## Distribution packages

- `xs-compiler` installs `/usr/bin/xs` and does not require a JVM.
- `xs-project` installs `/usr/bin/xs-project` and requires JRE 25 plus the `kotlin` scripting command.
- `xs-proj` installs the JVM-free `/usr/bin/xs-proj` manifest parser/validator.
- `xs-meta` is a metapackage depending on those packages and, when available, `xsfmt` and `xstidy`; it owns no compiler
  executable.
