<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Public roadmap

This page summarizes user-visible work that is still in progress for the X# compiler. It is intentionally short: detailed
implementation notes and internal design logs are not part of the public documentation set.

## Language and frontend

- Complete structural AST coverage for the documented X# syntax.
- Finish macro expansion from matched token fragments into AST declarations and statements.
- Complete HIR symbol, type, generic, method, operator, and interface checks.
- Complete expression type checking, mutability validation, and async/await validation.
- Continue the error-handling transition away from deprecated exception syntax toward `Result.Result<T, E>` and `@`
  propagation.

## Middle end

- Complete MIR lowering for statements, expressions, calls, Result propagation, async state machines, and drops.
- Finish borrow checking and drop-point validation.
- Grow MIR optimizations while preserving observable control flow, drop behavior, and diagnostics.
- Complete monomorphization and codegen-unit planning for generic functions and types.

## Intermediate formats

- Keep `.xhir`, `.xmir`, and `.xlil` as deterministic, human-readable text formats.
- Keep XHIR and XMIR independent from LLVM while aligned with the XLIL type/data vocabulary.
- Keep XLIL as the public backend input registry format; it is text-only and not bytecode.
- Extend the v0 parsers/writers/verifiers as new records and instructions are implemented.

## Backend and CLI

- Extend XLIL-to-LLVM lowering from declarations and the initial body subset to full function bodies.
- Emit object files and link project targets into native `.xse` executables, targeting ELF first and PE after ELF.
- Complete `xs build`, `xs check`, and `xs run` artifact handling and diagnostics.
- Keep the `compilerOptions.xsBackend` setting ready for the current LLVM backend and the future XS backend.

## Public APIs

- Stabilize the `.xsproj` C23 parser API for third-party tools.
- Grow the XLIL C23 API under `#include <xs/lil.h>` for external frontends and language implementations.
- Add AOT/JIT-facing APIs only when their behavior is implemented and testable.
