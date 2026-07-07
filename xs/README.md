<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs

Buildable X# compiler project.

This project owns the compiler CLI, `.xs` lexer/parser, AST, HIR, MIR, XLIL planning,
LLVM backend bridge and compiler tests. New C23 compiler implementation files live under
`xs/sources/`.
