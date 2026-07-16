<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs-project

`xs-project` is the Kotlin/JVM 25 resolver for programmable project files. It invokes the required external
`kotlin` command and emits a deterministic versioned project plan or exact source registry for the X# command-line
driver. It never reads or compiles `.xs` files. The compiler and
JRE are not embedded: JRE 25 or newer and an executable `kotlin` scripting command are runtime requirements.

Projects use one `xs.project.kts` file or the `xs.settings.kts` + `xs.build.kts` pair. Only
`project(name, channel, version)` and at least one source include are required. `source.include` names recursive directory
roots; source/test excludes may use `*`, `**`, and `?` glob patterns. The
resolved registry must contain exactly one case-sensitive `main.xs`, which is placed first as the entry. Setting
`XS_EXTENSION` replaces both the selected suffix and entry suffix. Optional sections cover authors, external modules,
multi-value settings such as `TARGET`, tests, compiler diagnostics, variables, and host-dependent `cfg(...)` branches. See `xs.project.kts` for the complete
working DSL example.

In split mode, `xs.settings.kts` is evaluated before `xs.build.kts`, but sections are not assigned to either filename.
Both scripts may use the full DSL; the second script extends the state produced by the first.

The canonical recursive source form is:

```kotlin
source {
  include("Sources")
  exclude("Sources/tests/**")
}
```

Include roots do not accept globs. Discovery is recursive relative to the project root, excludes run afterward, and the
deterministic result must contain exactly one case-sensitive `main.xs` by default. The
resolver emits that exact registry with `main.xs` first; it does not pass unresolved patterns to the compiler.
The internal registry also carries the evaluated `compiler {}` warning, warnings-as-errors, and verbose policy. The
JVM-free compiler may replace those values for one invocation through its corresponding command-line overrides.

```text
./xs_kts/gradlew --daemon --build-cache -p xs_kts test
./xs_kts/gradlew --daemon --build-cache -p xs_kts run --args="evaluate xs_kts"
```

Project files are build programs and must be trusted. They are executed by the external `kotlin` script runner; xs-project
does not maintain or embed a substitute Kotlin interpreter. Kotlin scripting is not presented as a security sandbox.

Users build from the project directory with `xs build`. The JVM-free compiler asks `xs-project` for source metadata and
then keeps all X# frontend/backend work inside `xs`. The `xs-project sources0` form is an internal compiler/resolver
protocol, not an alternative X# build command.
