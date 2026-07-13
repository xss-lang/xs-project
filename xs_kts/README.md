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
`project(name, channel, version)` and explicit source includes are required. Exactly one included file must be named
`main.xs`; it is always the entry. Optional sections cover authors, external modules, targets, tests, compiler diagnostics,
variables, and host-dependent `cfg(...)` branches.

```text
gradle -p xs_kts test
gradle -p xs_kts run --args="evaluate xs_kts"
```

Project files are build programs and must be trusted. They are parsed and executed by the real Kotlin compiler; xs-project
does not maintain a substitute Kotlin interpreter. Kotlin scripting is not presented as a security sandbox.

Users build from the project directory with `xs build`. The JVM-free compiler asks `xs-project` for source metadata and
then keeps all X# frontend/backend work inside `xs`. The `xs-project sources0` form is an internal compiler/resolver
protocol, not an alternative X# build command.
