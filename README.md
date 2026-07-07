<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs-project

`xs-project` is the LLVM-project-style monorepo root for the X# language and compiler family. The current focus is the X#
compiler (`xs`) and the public `.xsproj` manifest lexer/parser/model API (`xsproj`) that third-party tools can use.

This repository is experimental, but it is treated as serious compiler infrastructure: every step must remain buildable and
testable, the HIR/MIR layers must not depend on LLVM, and the documented compilation flow must be preserved.

## Quick start

Required core tools:

- CMake
- Ninja
- Clang / LLVM tools
- LLD
- BusyBox or non-GNU small coreutils alternatives are preferred

Default debug build:

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

On machines with prior OOM issues, use a 2GB virtual memory limit for build/test runs:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Check the example project:

```text
./build/clang-debug/xs check -proj tests/fixtures/example_project/MyApp.xsproj
```

## Monorepo directories

| Path | Status | Purpose |
| --- | --- | --- |
| `include/` | active | Shared public C headers across projects |
| `xs/` | active | X# compiler, CLI, lexer/parser, AST, macro, HIR, MIR, XLIL, LLVM backend infrastructure |
| `xsproj/` | active | Public C23 `.xsproj` parser/lexer/model API |
| `Spec/` | active source documentation | X# syntax and language behavior examples/spec files |
| `docs/` | active documentation | Architecture, build, CLI, backend, roadmap, and implementation status |
| `tests/` | active | C-based unit and integration tests |
| `xsfmt/` | future | Rust nightly + Serde formatter |
| `xstidy/` | future | Rust nightly + Serde linter |
| `xs-analyzer/` | future | Rust language server + TypeScript VS Code extension |
| `xs-backend/` | future | C23-heavy native XS Backend |

## Compiler pipeline

Documented order:

```text
.xs sources
    → lexing and parsing
    → structural AST
    → macro expansion
    → HIR and dependency graph
    → type checking
    → MIR
    → borrow checker
    → MIR optimization
    → monomorphization
    → codegen units
    → XLIL
    → LLVM IR / future XS Backend lowering
    → optimization
    → object code
    → linking
```

This order is not just documentation; it is an implementation boundary. Missing frontend behavior must not be invented in
the backend. HIR and MIR do not depend on the LLVM API; the backend entry language is the target-independent XLIL layer.

## Current working features

- C23-based, Clang/LLVM-focused build system
- Anti-GNU CMake/toolchain checks
- `.xsproj` manifest parser/lexer/model public C API
- X# lexer and structural AST parser
- Synthetic reparse and expanded-view infrastructure for declaration/statement macro expansion
- HIR symbol table plus import/name/type resolution bootstrap
- Primitive type metadata and nominal user-defined type resolution
- Literal initializer/assignment/return checks
- MIR model API, text writer, borrow-check skeleton, and a few MIR optimization passes
- XHIR/XMIR structured text writer and header parser bootstrap in Rust `xslang`
- XLIL model, assembly-like text writer/parser/verifier, and limited MIR → XLIL lowering core
- LLVM context/module/target/object/link infrastructure

For the detailed current status, see [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md).

## Release policy

Numbered X# releases will begin after LLVM IR generation is working. Until then, the root [CHANGELOG.md](CHANGELOG.md) file is
kept as an `Unreleased` development log.

## CLI summary

Currently supported command shapes:

```text
xs check -proj MyApp.xsproj
xs build -proj MyApp.xsproj
xs build --output hir -proj MyApp.xsproj
xs build --output mir -proj MyApp.xsproj
xs build --output xlil -proj MyApp.xsproj
xs build --output hir -file Main.xs
xs build --output mir -file Main.xs
xs build --output xlil -file Main.xs
xs build --hir -file Main.xs
xs build --mir -file Main.xs
xs build --xlil -file Main.xs
xs run -proj MyApp.xsproj
```

The `-proj` forms run through a project manifest. The `-file` forms are reserved for direct single-file/intermediate input
flows; they are accepted by the CLI and may intentionally emit diagnostics until the corresponding pipeline is connected.
Intermediate output extensions:

- `.xhir`: human-readable HIR text, intended for direct semantic inspection and review
- `.xmir`: human-readable MIR text, intended for direct control-flow/borrow inspection and review
- `.xlil`: XLIL text registry

`.xhir`, `.xmir`, and `.xlil` are intentionally text formats, not opaque serialized compiler state. `.xhir` and `.xmir`
must remain human-readable even when their official record grammar becomes stricter. They are not assembly-like: XHIR is a
structured semantic tree/record dump, and XMIR is a structured control-flow/analysis dump. `.xlil` will never be a binary
format. Current XLIL text is assembly-like and uses directive/label/value records such as `.xlil module`, `.extern`, `.func`,
`bb0.entry:`, `%0:i64 = const 42`, `br bb1`, `ret %0`, and `.end`.

## Development rules

- The primary implementation language is C23.
- Do not use `#include <stdbool.h>` in new/touched C code; use C23 `bool`.
- Prefer `nullptr` over `NULL` in new/touched C code.
- Use CMake; do not use Meson.
- GNU C compiler, GNU Make, GNU binutils fallbacks, and GNU C dialects are rejected.
- Do not add persistent shell scripts; use Java source-file tools or D for automation.
- Keep files under 1000 lines; prefer smaller modules when a component starts to sprawl.
- Prefer non-GNU tools such as `busybox wc` for line counting.
- Use `ulimit -v 2097152` during test/build runs to reduce OOM risk.

For broader contribution and workflow rules, see [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## Documentation map

- [docs/README.md](docs/README.md): documentation entry point
- [docs/BUILDING.md](docs/BUILDING.md): build, test, toolchain, and OOM notes
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): compiler architecture and stage boundaries
- [docs/CLI.md](docs/CLI.md): CLI contract and current status
- [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md): detailed implementation status
- [docs/TODO.md](docs/TODO.md): X# v0 decisions and tracking list
- [docs/MONOREPO.md](docs/MONOREPO.md): monorepo selection model
- [docs/LLVM_BACKEND.md](docs/LLVM_BACKEND.md): LLVM backend infrastructure
- [docs/XLIL.md](docs/XLIL.md): XLIL text registry and public API direction

## License

For license and notice information, see the root `LICENSE.txt` and `NOTICE.txt` files.
