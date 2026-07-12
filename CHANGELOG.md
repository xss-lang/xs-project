<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Changelog

This file summarizes user-visible and developer-visible changes in the xs-project repository.

The 0.0.x development period is the pre-1.0 xs-project compiler infrastructure line. It does not imply a complete X#
source-to-native executable pipeline.

## Unreleased

- The C23 lexer/parser now accepts `#[...]`/`#![...]` attribute syntax and top-level `extern "ABI" { fn ...; }` blocks.
- `Spec/Attrs.xs` and `Spec/CFFI.xs` document the standard attribute registry and the explicit opt-in CFFI surface.
- `format_args!` is now validated as a built-in macro instead of a Stdio export.
- Stdio macro validation now accepts Rust 1.57-style `write!` and `writeln!` forms plus common Rust formatting specs such
  as `{:?}`, `{:#?}`, `{:08x}`, and `{:_>8}` while still checking placeholder counts.
- `Optional<T>` now resolves as an implicit compiler-provided `Optional` import alias for `STD.Optional.Optional<T>`, with
  value constructors canonically named `STD.Optional.None` and `STD.Optional.Some(...)`.
- Legacy exception syntax is now deprecated. `throws`, `throw`, and `try` remain parseable but produce warnings; new code
  should use `Result.Result<T, E>` and postfix `@` propagation.
- The C23 HIR expression checker now accepts postfix `@` inside functions returning `Result.Result<T, E>` or
  `Result<T, E>` and rejects it outside Result-returning functions. Direct same-file function call operands are now also
  checked to return a Result type.
- `Spec/` examples now use Result-style error handling for active examples. Legacy `throws`/`throw`/`try`/`catch` syntax is
  kept only in deprecated legacy notes.

## 0.0.5 - 2026-07-12

### Added

- Imported `Stdio` macros now include `print!`, `println!`, `eprint!`, `eprintln!`, and `format!`, with Rust 1.57-style
  newline-only `println!()`/`eprintln!()` forms.
- `Spec/Stdio.xs` now documents `std.Stdin()` line input examples, and `Spec/Result.xs` documents the explicit `Result`
  model, postfix `@` propagation, `expect`, and `unwrap`.
- The C23 lexer/parser now accepts postfix `@` as a structural Result propagation expression; semantic checking and lowering
  are intentionally left for later Result work, and HIR expression checking reports that gap explicitly instead of silently
  accepting it.
- Rust `xslang` XHIR now models and round-trips Result propagation with a `propagate` expression record.
- Rust `xslang` type checking and inference now implement the first Result propagation rule: `Result<T, E>@` yields `T`,
  and the enclosing function must return `Result<_, E>`. HIR-to-MIR lowering still intentionally rejects propagation until
  control-flow lowering for error returns exists.
- Rust `xslang` now includes an explicit Result propagation desugar pass. Surface `value@` is translated into a
  Result-match/early-return intent model before MIR lowering work begins, so backend stages do not need to treat `@` as a
  primitive operation.
- Rust `xslang` MIR lowering now exposes a `DesugaredFunction` entry point. Desugared functions without ResultMatch lower
  through the existing HIR path, while explicit ResultMatch nodes are rejected until MIR Result control-flow lowering exists.
- Rust `xslang` XHIR text writing can now emit desugared functions, including explicit `result_match` records for `@`
  desugar output.
- Rust `xslang` treats single-argument `Result<T>`/`Result.Result<T>` as using the standard `Result.Error` error type for
  propagation type checking and desugaring.
- The C23 HIR type resolver now recognizes the standard wrapper type names `Optional<T>`, `Result.Result<T>`,
  `Result.Result<T, E>`, shorthand `Result<T, E>`, and `Result.Error` without requiring a user-defined project symbol.
- C MIR, XLIL, MIR optimization, and LLVM lowering now support signed i64 bitwise operations, shifts, inequality, and
  signed ordering comparisons.
- Plain source native builds now accept explicit `Long` and inferred i32-compatible local bindings before the final
  `return`, and those locals can be returned or used by the current i32 expression/condition lowering slice.
- Plain source native builds now accept explicit and inferred `Bool` local bindings for the current `if` condition lowering
  slice.
- C MIR, XLIL, LLVM lowering, and the source-native slice now support materialized boolean negation through `not.bool`, so
  `Bool` local initializers can use unary `!` in the current native path.
- The lexer/parser and source-native `Long` expression slice now support bitwise `^`, lowering to the existing `xor.i32`
  MIR/XLIL/LLVM path.
- C MIR constant folding now covers the i32 arithmetic, bitwise, shift, equality, and signed comparison family used by the
  current source-native `Long` slice.
- C MIR constant folding now turns `branch_if` with a known `const.bool` condition into a direct `goto`, allowing CFG cleanup
  to remove the dead branch.
- Source-native `if` expressions with a syntactically constant boolean condition now lower only the selected branch in the
  current narrow native slice.
- Source-native `if` expressions with constant i32 literal/arithmetic comparisons now also lower only the selected branch.

### Changed

- Source-native builds now run MIR constant folding and CFG cleanup before MIR is lowered to XLIL.

### Known gaps

## 0.0.4 - 2026-07-12

### Added

- Native executable artifacts now use the `.xse` extension; the first implemented container target is Linux ELF, with PE
  planned after ELF support.
- `xs build -file <Main.xs>` and `xs build -proj <App.xsproj>` can now produce `.ll`, `.o`, and `.xse` artifacts for the
  first supported source-native entry slice: top-level `main` returning `Long` with i32-range literals, `+`, `-`, `*`, and
  one top-level `if` expression over i32 comparisons.
- XLIL, MIR, LLVM lowering, and source-native `if` conditions now support signed `ne.i32` inequality.
- XLIL, MIR, LLVM lowering, and source-native `Long` return expressions now support bitwise `and.i32` and `or.i32`.
- XLIL, MIR, LLVM lowering, and source-native `Long` return expressions now support signed i32 `shl.i32` and arithmetic
  `shr.i32` shifts.
- C MIR, XLIL, MIR optimization, and LLVM lowering now support signed `div.i64` and `rem.i64`, completing the current
  direct i64 arithmetic family.
- C MIR to XLIL lowering now supports `const.bool`, and source-native `if` conditions accept `true`/`false`.
- XLIL, MIR, LLVM lowering, and the source-native slice now support signed `div.i32`.
- XLIL, MIR, LLVM lowering, and the source-native slice now support signed `rem.i32`.
- Source-native `Long` return expressions now support unary `-`.
- Source-native `if` conditions now support unary `!` by swapping branch targets.
- Source-native `Long` return expressions now support unary `+`, matching the documented narrow arithmetic slice.
- XLIL v0 and direct LLVM lowering now support signed `i32` comparison instructions: `lt.i32`, `le.i32`, `gt.i32`,
  and `ge.i32`.
- C MIR to XLIL body lowering now creates all destination blocks before lowering terminators, so `br` and `br_if` can target
  later blocks.

### Changed

### Known gaps

## 0.0.3 - 2026-07-10

### Added

- Direct XLIL native builds now run the LLVM verification and optimization pipeline before writing LLVM IR and object
  artifacts.
- XLIL v0 and the LLVM backend now support `add.i32`, `sub.i32`, `mul.i32`, and `eq.i32` instructions.

### Changed

### Known gaps

## 0.0.2 - 2026-07-10

### Added

- XLIL v0 now has explicit `.param %rN:type` body records and registry-verified direct `call` records.
- The public C23 XLIL API verifies direct call signatures, and the LLVM backend lowers XLIL parameters and calls.
- The C23 XLIL model and LLVM backend now support `add.i64`, `sub.i64`, `mul.i64`, and `eq.i64` instructions.
- Direct `xs build --xlil -file` now emits LLVM IR, an object file, and a local native executable for a defined
  `.func main : () -> i32` XLIL entry point.
- XLIL v0 now supports `%rN:i32 = const.i32 N` for direct native process exit values.
- Rust `xslang` now carries `const.i32` through XMIR text, MIR verification, MIR → XLIL lowering, and XLIL v0 text.
- GitHub Actions now verifies C23 debug and sanitizer builds, Rust formatting/tests, and patch hygiene.
- X# source comments are line-only; `include!` now enters the driver after the initial structural AST parse.

### Changed

### Known gaps

## 0.0.1 - 2026-07-09

### Added

- LLVM-project-style monorepo layout.
- `xs` compiler project and `xsproj` public `.xsproj` manifest parser/model API.
- X# lexer, structural AST parser, and base diagnostic system.
- Macro validation, token expansion, statement/declaration reparsing, and expanded-view infrastructure.
- Initial HIR symbol table, import resolution, name resolution, and type resolution infrastructure.
- Generic arity, duplicate generic parameter, and generic interface constraint checks.
- Primitive type metadata for `bool`, `byte`, `sbyte`, `char`, integer types, supported float types, and `str`.
- MIR model API, MIR writer, borrow-check skeleton, and initial optimizer passes.
- XLIL model, assembly-like text writer/parser/verifier, and limited MIR-to-XLIL body lowering.
- LLVM backend infrastructure for context/module/target/data layout management, signature lowering, LLVM IR text emission,
  object emission, and linker abstraction.
- CLI paths for `xs check -proj ...` and `xs build --output hir|mir|xlil -proj ...`.
- CLI recognition for direct file forms: `xs build --output hir|mir|xlil -file ...` and
  `xs build --hir|--mir|--xlil -file ...`.
- Direct `.xlil` inputs with supported version/module headers and top-level `.extern`/`.func` signatures can produce LLVM IR
  declarations.
- Public XLIL C23 API can parse v0 text registry files and expose read-only function signature metadata.
- LLVM backend can lower the first XLIL body subset: parameters, `const i64`, `const.bool`, direct calls, `br`, `br_if`,
  `ret`, and `ret %rN` functions.
- Documentation clarifies that `.xhir`, `.xmir`, and `.xlil` are human-readable text formats, not binary or opaque
  serialized compiler state.
- XHIR and XMIR documentation now separates their structured text design from XLIL's assembly-like registry format.
- Rust `xslang` gained initial structured XHIR/XMIR text writers plus parser subsets.
- XHIR parsing now covers the checked-function subset emitted by the Rust writer.
- XMIR tests now cover goto, local return values, unreachable terminators, and the current local statement records.
- XLIL text now starts with `.xlil version 0` before module records.
- MIR/XMIR can now carry explicit function parameter/return types, optional XLIL local value types, and `const.i64`
  statements; Rust MIR → XLIL lowering can emit signatures, `const`, and `ret %rN` for typed i64 local returns.
- Rust `xslang` XLIL gained typed `call` instructions plus writer/parser/verifier coverage; MIR/XMIR can carry typed call
  statements that lower when their arguments already have XLIL values.
- Rust `xslang` gained a MIR structural verifier for duplicate ids, missing terminators, and unknown local/block references.
- Rust `xslang` optimizer gained a verified entry point that checks MIR before and after optimization.
- XMIR text support gained optimizer analysis writer/parser coverage for optimization pass reports.
- XMIR text support gained structural verifier analysis writer/parser coverage.
- XHIR text support gained type-check analysis writer/parser coverage.
- Root README and strengthened documentation set under `docs/`.
- `xs --version` reports the current compiler version.
- XLIL text uses register-style value names such as `%r0`.

### Changed

- Module selected-import syntax uses `from` instead of `froms`.
- `byte` resolves to `u8` at HIR level, and `sbyte` resolves to `i8`.
- `char` is documented as a 16-bit UTF-16 code unit.
- `str` is treated as UTF-16 and unbounded up to runtime allocator and target environment limits.
- HIR and MIR are documented as independent from LLVM and tied to the XLIL vocabulary instead.
- XLIL text moved away from C-like function records toward assembly-like `.xlil`, `.extern`, `.func`, label, typed SSA
  value, `br`, and `ret` records.
- Structural parser can consume a `>>` token as two closing `>` tokens when ending generic type/parameter lists.
- `match` examples and tests use the parenthesized subject form, such as `match (value) { ... }`.
- X# no longer exposes `float16` or `double` as language primitive type names.

### Known gaps

- Full AST macro replacement does not yet perform in-place parent-child rewrites.
- General expression type inference, overload resolution, and nominal interface membership checks are incomplete.
- MIR statement/expression lowering, exception edges, and async state machine generation are incomplete.
- Borrow checker is not yet a full region/loan/move/drop model.
- End-to-end `xs build`/`xs run` object/link/native executable flow is incomplete.
- Direct XLIL build mode currently lowers only the supported initial body subset.
