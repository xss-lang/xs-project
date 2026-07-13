<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs-project

`xs-project` is the Kotlin/JVM 25 resolver for programmable project files. It invokes the required external
`kotlin` command and emits a deterministic versioned project plan or exact source registry for the X# command-line
driver. It never reads or compiles `.xs` files. The compiler and
JRE are not embedded: exactly JRE 25 and an executable `kotlin` scripting command are runtime requirements.

Projects use one `xs.project.kts` file or the `xs.settings.kts` + `xs.build.kts` pair. Only
`project(name, channel, version)` and at least one source include are required. Source includes accept concrete paths or
`*`, `**`, and `?` glob patterns; excludes are applied afterward. The resolved registry must contain exactly one
case-sensitive `main.xs`, which is placed first as the entry. Optional sections cover authors, external modules, targets,
tests, compiler diagnostics, variables, and host-dependent `cfg(...)` branches. See `xs.project.kts` for the complete
working DSL example.

The canonical recursive source form is:

```kotlin
sources {
  include("sources/**/*.xs")
  exclude("sources/tests/**")
}
```

Concrete includes may be mixed with `*`, `**`, and `?` patterns. Matching happens relative to the project root,
excludes run after includes, and the deterministic result must contain exactly one case-sensitive `main.xs`. The
resolver emits that exact registry with `main.xs` first; it does not pass unresolved patterns to the compiler.
The internal registry also carries the evaluated `compiler {}` warning, warnings-as-errors, and verbose policy. The
JVM-free compiler may replace those values for one invocation through its corresponding command-line overrides.

```text
gradle -p xs_kts test
gradle -p xs_kts run --args="evaluate xs_kts"
```

Project files are build programs and must be trusted. They are executed by the external `kotlin` script runner; xs-project
does not maintain or embed a substitute Kotlin interpreter. Kotlin scripting is not presented as a security sandbox.

Users build from the project directory with `xs build`. The JVM-free compiler asks `xs-project` for source metadata and
then keeps all X# frontend/backend work inside `xs`. The `xs-project sources0` form is an internal compiler/resolver
protocol, not an alternative X# build command.
