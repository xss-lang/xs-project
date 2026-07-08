<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Changelog

This file summarizes user-visible and developer-visible changes in the xs-project repository.

Versioned releases will begin after LLVM IR generation becomes available. Until then, changes are tracked under
`Unreleased`; this heading is a development log, not a release promise.

## Unreleased

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
- LLVM backend infrastructure for context/module/target/data layout management, signature lowering, object emission,
  and linker abstraction.
- CLI paths for `xs check -proj ...` and `xs build --output hir|mir|xlil -proj ...`.
- CLI recognition for direct file forms: `xs build --output hir|mir|xlil -file ...` and
  `xs build --hir|--mir|--xlil -file ...`.
- Documentation clarifies that `.xhir`, `.xmir`, and `.xlil` are human-readable text formats, not binary or opaque
  serialized compiler state.
- XHIR and XMIR documentation now separates their structured text design from XLIL's assembly-like registry format.
- Rust `xslang` gained initial structured XHIR/XMIR text writers plus parser subsets.
- XHIR parsing now covers the checked-function subset emitted by the Rust writer.
- XMIR tests now cover goto, local return values, unreachable terminators, and the current local statement records.
- XLIL text now starts with `.xlil version 0` before module records.
- MIR/XMIR can now carry optional XLIL local value types and `const.i64` statements; Rust MIR → XLIL lowering can emit
  `const` plus `ret %N` for typed i64 local returns.
- Rust `xslang` gained a MIR structural verifier for duplicate ids, missing terminators, and unknown local/block references.
- Rust `xslang` optimizer gained a verified entry point that checks MIR before and after optimization.
- XMIR text support gained optimizer analysis writer/parser coverage for optimization pass reports.
- XMIR text support gained structural verifier analysis writer/parser coverage.
- XHIR text support gained type-check analysis writer/parser coverage.
- Root README and strengthened documentation set under `docs/`.

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
- Direct XLIL build mode (`xs build --xlil -file ...`) is not implemented yet.
