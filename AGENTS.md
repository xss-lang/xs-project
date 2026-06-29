# XS project rules

## Project shape

- This repository implements the X# compiler.
- C23 is the primary implementation language.
- New compiler components should be written in C under `sources/` by default.
- Optional Rust code is allowed only when there is a concrete technical reason; keep it isolated under `sources/rust/` and do
  not break the C API.
- C and header files must stay at or below 500 lines.
- Keep subsystems modular. Split growing files before they become hard to review.

## Build system and toolchain

- Use CMake, not Meson.
- Use Clang/LLVM tooling and LLD.
- The project is intentionally anti-GNU:
  - do not add GCC-only behavior;
  - do not require GNU Make;
  - do not use GNU C dialects such as `gnu23`;
  - do not add `_GNU_SOURCE`;
  - do not rely on GNU binutils.
- NASM `.asm`/`.inc` files may be used only when they are optional and portable across supported targets. Do not assume x86
  only; ARM64 and x86-64 compatibility matter.

## Formatting and static analysis

- Keep `.clang-format`, `.clangd` and `.clang-tidy` aligned with the project’s C23/Clang setup.
- Project C macros that affect formatting should be registered in `.clang-format`.
- Do not use `#include <stdbool.h>`; use C23 `bool`.
- Do not format `CMakeLists.txt` mechanically. Patch it carefully and preserve user edits.

## X# syntax and manifests

- `.xsproj` files are XS project manifests.
- Do not replace `.xsproj` syntax with JSON, YAML, TOML or XML.
- `.xsproj` parser/lexer/model code lives under `sources/xsproj/`.
- `.xsproj` lexer/parser code is separate from `.xs` lexer/parser code. `.xsproj` supports `//` and `///` line comments
  only; multiline comments are not supported.
- `.xsproj` parsing is a public C23 API surface, similar in role to a JSON parser for external tools:
  `#include <xs/project.h>`.
- Before modifying X# syntax handling or XS source examples, read the relevant files under `./XS/Syntax/`.
- Do not invent syntax that is not documented under `./XS/Syntax/`.
- `compilerOptions.addFiles.entry` is the program entry source file.
- Other files inside `addFiles` are additional source files.
- Valid `appRelease` values are `ALPHA`, `BETA` and `STABLE`.
- `compilerOptions.xsBackend` accepts `"LLVM"` and `"XS"`. Unsupported backend behavior should be diagnosed, not silently
  guessed.

## Language and compiler decisions

- `docs/TODO.md` is not a question backlog anymore; it is the X# v0 decision and implementation tracking document.
- For language semantics, runtime, ABI, MIR, backend and tooling decisions, follow `docs/TODO.md`.
- Syntax and XLIL must stay tied to documented forms. Do not invent undocumented syntax or undocumented XLIL formats.
- For areas other than Syntax and XLIL, small missing implementation details may be filled in consistently with
  `docs/TODO.md` and `docs/IMPLEMENTATION.md`.
- When a larger rule is still undocumented, decide it without waiting for more user documentation, document the decision in
  `docs/TODO.md`, and keep documented X# rules higher priority. Rust, TypeScript, C#, C23 and target assembly conventions may
  be used as design inspiration where they fit X#.
- `byte` maps to `u8`; `sbyte` maps to `i8`.
- `char` is a 16-bit UTF-16 code unit.
- `str` is UTF-16 and its length is limited by the platform/runtime representation, not by an arbitrary compiler limit.
- HIR resolves language types. `bool` becomes `i1` at the HIR/low-level type boundary.
- X# uses nominal typing. User-defined type identity is name/symbol based; identical field structure does not make two types
  compatible.
- HIR and MIR must not depend on LLVM APIs. They may depend on the target-independent XLIL type/data vocabulary.
- Planned public APIs are `#include <xs/hir/jit.h>` for the HIR baseline JIT, `#include <xs/mir/jit.h>` for the MIR
  performance JIT and `#include <xs/lil.h>` for AOT and XLIL generation.
- Third-party languages may use the public HIR/MIR JIT and XLIL/AOT APIs, but do not add fake JIT semantics before those
  layers are implemented.

## Compiler pipeline

Follow the documented pipeline in order:

1. lexing and parsing
2. structural AST
3. macro expansion
4. HIR
5. type checking
6. MIR
7. borrow checking
8. MIR optimization
9. monomorphization
10. codegen units
11. XLIL
12. LLVM IR or future XS Backend lowering
13. optimization
14. object code
15. linking

Do not skip ahead by inventing semantics. If a step is not implemented yet, keep the project buildable and add the next
testable piece.

## CLI contracts

- The documented project commands are:
  - `xs check -proj MyApp.xsproj`
  - `xs build -proj MyApp.xsproj`
  - `xs run -proj MyApp.xsproj`
- Intermediate output options are:
  - `xs build --output hir -proj MyApp.xsproj`
  - `xs build --output mir -proj MyApp.xsproj`
  - `xs build --output xlil -proj MyApp.xsproj`
- Future direct XLIL build entry:
  - `xs build --xlil -file foo.xlil`
- `.xhir` is HIR code, `.xmir` is MIR code and `.xlil` is XLIL code.

## Verification

- Keep every milestone buildable and testable.
- Prefer small patches over one giant compiler patch.
- Run focused tests for the touched subsystem.
- When practical, run:
  - `cmake --build --preset clang-debug`
  - `ctest --preset clang-debug --output-on-failure`
  - `git diff --check`
- Parser tests can consume memory if they regress; use a memory limit when appropriate.
