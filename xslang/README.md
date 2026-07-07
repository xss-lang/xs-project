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
