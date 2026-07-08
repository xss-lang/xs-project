<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xslang

`xslang` is the future Rust compiler-core crate for X#.

It is intentionally separate from the current C23 lexer, parser, structural AST, `.xsproj` parser, and LLVM infrastructure.
New semantic-analysis, typed-HIR, MIR, borrow-checker, optimizer, monomorphization, and lowering work belongs here.

The crate is not wired into the C23 compiler driver yet. The initial modules are small, deterministic, and unit-tested so the
Rust core can grow without splitting one compiler layer across C and Rust.

Current core slices include:

- HIR type-checking helpers and structured XHIR text support.
- A first HIR to MIR lowering bridge for void functions, `Int` locals, `Int` literals, and local returns.
- MIR structural verification, borrow-checking, optimizer scaffolding, and structured XMIR parser/writer modules.
- MIR and XLIL `add.i64`/`sub.i64`/`mul.i64` plus `const.bool`/`eq.i64` support, including XMIR/XLIL round-tripping and MIR constant folding for constant operands.
- MIR to XLIL lowering for typed `const.i64`, `const.bool`, `add.i64`, `sub.i64`, `mul.i64`, `eq.i64`, call, branch, and return records.
- Monomorphization and codegen-unit planning models that do not depend on LLVM.
