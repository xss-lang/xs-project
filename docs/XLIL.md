<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# XLIL

XLIL, the X# Low-Level Intermediate Language, is the compiler project's official low-level intermediate language.

XLIL sits before LLVM IR and is designed as the shared input point for all native backends.

## Purpose

- Provide a standard low-level intermediate language for X#.
- Remove direct LLVM IR coupling from frontend and middle-end layers.
- Give every backend a shared intermediate input.
- Provide the foundation for the future XS Backend.
- Allow other programming languages to generate XLIL and use the XS native backend pipeline.

## Shape

XLIL is:

- lower level than CIL,
- higher level than raw assembly,
- assembly-like without being assembly,
- target-independent,
- backend-independent,
- designed for native machine-code generation,
- not bytecode,
- not a virtual-machine instruction stream,
- always text,
- never binary.

## Registry format

`.xlil` files store registry-style records. Module records, function declarations, function definitions, types, and
future body records are stored as text.

An `.xlil` file is not executable bytecode. It is backend input.

Current text records intentionally look closer to assembly than to C-like declarations:

```text
.xlil version 0
.xlil module App
.extern Next : (i64) -> i64
.func Answer : () -> i64
bb0.entry:
  %r0:i64 = const 42
  %r1:i64 = call Next(%r0)
  ret %r1
.end
```

Format notes:

- `.xlil version 0` starts a registry file and declares the XLIL text grammar version that `xs build` should parse.
- `.xlil module <name>` declares the module name after the version header.
- `.extern <symbol> : (<params>) -> <return>` declares an external function.
- `.func <symbol> : (<params>) -> <return>` starts a function body.
- `bbN.<label>:` starts a basic block.
- `%rN:type` names a typed SSA value.
- `%rN:bool = const.bool true|false` creates a boolean SSA value.
- `%rN:type = call <symbol>(%rA, %rB)` calls another function and stores a typed result.
- `call <symbol>(%rA, %rB)` calls a void function and discards the result.
- `br bbN` transfers control to another basic block.
- `br_if %rN, bbA, bbB` branches to `bbA` when the `bool` condition `%rN` is true, otherwise to `bbB`.
- `ret` and `ret %rN` are the current return terminators.
- `.end` closes a function body.

Version `0` is the only supported XLIL version today. The header exists so `xs build --xlil -file <input.xlil>` can select
the correct XLIL grammar as the format evolves. It is not a bytecode VM version and does not make `.xlil` binary.

## Public API

XLIL will expose a stable C23 API through:

```c
#include <xs/lil.h>
```

This header is the public API surface for AOT production and XLIL registry generation.

The API is intended to let:

- other programming languages generate XLIL,
- third-party compilers target XLIL,
- alternative frontends exist,
- every XLIL-producing compiler reuse the same backend infrastructure.

The C23 API currently includes module construction, text writing, v0 text parsing, and read-only function/body inspection.
Direct `xs build --xlil -file <input.xlil>` uses this parser API before emitting temporary LLVM IR. The command is intended
to produce a native executable later, after object emission and linking are connected.
