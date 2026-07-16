<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xslang

`xslang` is the future Rust compiler-core crate for X#.

It is intentionally separate from the current C23 lexer, parser, structural AST, `.xsproj` parser, and LLVM infrastructure.
New semantic-analysis, HIR (coordinated THIR and XHIR sides), MIR, borrow-checker, optimizer, monomorphization, and lowering
work belongs here.

The crate is connected to the C23 compiler driver through a narrow C ABI. The C23 frontend supplies structural syntax to a
Rust compiler-core session; the session can emit versioned XHIR, XMIR, and XLIL program text and can feed the existing native
backend when every required lowering stage succeeds.
Direct-IR sessions also accept complete XHIR or XMIR v0 program text, validate the appropriate typed/MIR invariants, and
produce verified XLIL text for the C23 LLVM backend without introducing an LLVM dependency into HIR or MIR.

Current core slices include:

- HIR type-checking helpers for the THIR side and structured XHIR model/text support for the operational side.
- A first HIR to MIR lowering bridge for void functions, `Int` locals, `Int` literals, and local returns.
- MIR structural verification, borrow-checking, optimizer scaffolding, and structured XMIR parser/writer modules.
- MIR and XLIL `add.i64`/`sub.i64`/`mul.i64` plus `const.bool`/`eq.i64` support, including XMIR/XLIL round-tripping and MIR constant folding for constant operands.
- MIR and XLIL conditional terminator infrastructure: MIR `branch_if`, XLIL `br_if`, verifier checks, and MIR-to-XLIL lowering for already-lowered `bool` conditions.
- MIR to XLIL lowering for typed `const.i64`, `const.bool`, `add.i64`, `sub.i64`, `mul.i64`, `eq.i64`, call, unconditional/conditional branch, and return records.
- Monomorphization and codegen-unit planning models that do not depend on LLVM.
