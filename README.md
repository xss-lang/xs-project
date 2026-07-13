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

- CMake 3.31 or newer
- Ninja
- Clang / LLVM tools
- LLD
- Rustup and Cargo; `xslang/rust-toolchain.toml` selects the pinned nightly compiler core toolchain
- Optional helper tools such as `fd`, `rg`, `bat -p`, `sd`, and `busybox wc` are useful for development

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
| `xslang/` | active | Rust semantic compiler core linked into the C23 driver through a versioned bulk AST boundary |
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
- Clang/LLVM-oriented CMake/toolchain checks
- `.xsproj` manifest parser/lexer/model public C API
- X# lexer and structural AST parser
- Synthetic reparse and expanded-view infrastructure for declaration/statement macro expansion
- HIR symbol table plus import/name/type resolution bootstrap
- Primitive type metadata and nominal user-defined type resolution
- Literal initializer/assignment/return checks
- MIR model API, text writer, structural verifier, borrow-check skeleton, and a few MIR optimization passes
- XHIR/XMIR structured text writer and header parser bootstrap in Rust `xslang`
- XLIL model, assembly-like text writer/parser/verifier, and limited MIR → XLIL lowering core
- LLVM context/module/target/object/link infrastructure

For the detailed current status, see [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md).

## Specification examples

The public language examples live under [Spec/](Spec). Smaller files document syntax and semantic intent for declarations,
types, macros, standard modules, control flow, and project manifests. Larger one-file completed-language sketches live under
[Spec/Programs/](Spec/Programs); they are design fixtures, not promises that the current partial compiler can compile every
example today.

## Release policy

The project is now in the `0.1.4` development period. Supported source functions cross the C23 structural-AST boundary into
Rust typed HIR, verified and optimized MIR, and XLIL before the public C23 XLIL model drives LLVM native `.xse` emission.
Rust compiler-core control flow now includes conditional loops, post-test `do`/`while` sugar, loop jumps, and statement
`match`; mutable locals cross CFG edges through explicit target-independent MIR storage operations. Prefix and postfix
increment/decrement preserve their distinct result values through the current native local-storage slice. It does not imply
that the complete X# language is executable yet. Earlier releases are compiler infrastructure snapshots.

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
format. Current XLIL text is assembly-like and starts with `.xlil version 0`, then uses directive/label/value records such
as `.xlil module`, `.extern`, `.func`, `bb0.entry:`, `%r0:i64 = const 42`, `br bb1`, `ret %r0`, and `.end`.

## Development rules

- The primary implementation language is C23.
- Do not use `#include <stdbool.h>` in new/touched C code; use C23 `bool`.
- Prefer `nullptr` over `NULL` in new/touched C code.
- Use CMake; do not use Meson.
- The supported build path is Clang, Ninja, LLVM tools, and LLD.
- Do not add persistent shell scripts; use Java source-file tools or D for automation.
- Keep files under 1000 lines; prefer smaller modules when a component starts to sprawl.
- Use `busybox wc` or another compact line-counting tool when checking file length.
- Use `ulimit -v 2097152` during test/build runs to reduce OOM risk.

For broader contribution and workflow rules, see [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)

## Documentation map

- [docs/README.md](docs/README.md): documentation entry point
- [docs/BUILDING.md](docs/BUILDING.md): build, test, toolchain, and OOM notes
- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): compiler architecture and stage boundaries
- [docs/CLI.md](docs/CLI.md): CLI contract and current status
- [docs/IMPLEMENTATION.md](docs/IMPLEMENTATION.md): detailed implementation status
- [docs/SPEC.md](docs/SPEC.md): guide to the `Spec/` language examples
- [docs/TODO.md](docs/TODO.md): public roadmap
- [docs/MONOREPO.md](docs/MONOREPO.md): monorepo selection model
- [docs/LLVM_BACKEND.md](docs/LLVM_BACKEND.md): LLVM backend infrastructure
- [docs/XLIL.md](docs/XLIL.md): XLIL text registry and public API direction

## License

For license and notice information, see the root `LICENSE.txt` and `NOTICE.txt` files.
