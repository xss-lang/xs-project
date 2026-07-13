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

sources {
  include("sources/main.xs")
  include("sources/library.xs")
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

`project(name, channel, version)` and explicit source includes are required. Exactly one included source must have the
case-sensitive filename `main.xs`; it is always the entry file. Include records name concrete project-relative `.xs`
files rather than glob patterns. Optional DSL sections cover variables, authors, external modules, targets, tests,
diagnostic settings, and host-dependent `cfg(...)` branches.

BSD hosts are members of both the `BSD` and `UNIX` families. Consequently, `cfg(FAMILY == BSD)` and
`cfg(FAMILY == UNIX)` are both true on FreeBSD, OpenBSD, and NetBSD.

Argument-free `xs build`, `xs check`, and `xs run` search the current directory and its parents through `xs-project`.
The resolver evaluates Kotlin and returns an exact source registry; it never reads or compiles `.xs` files. The JVM-free
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
