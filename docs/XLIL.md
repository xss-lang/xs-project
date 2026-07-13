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
- Keep the compiler's target-independent IR boundary separate from LLVM APIs.
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
- `.param %rN:type` maps a signature parameter to a body register. Records occur before the first basic block, in
  signature order, and repeat the signature type.
- `.slot %sN:type` declares a typed function-local stack slot after parameters and before the first basic block.
- `bbN.<label>:` starts a basic block.
- `%rN:type` names a typed SSA value.
- `%rN:bool = const.bool true|false` creates a boolean SSA value.
- `%rN:i32 = const.i32 N` creates a signed 32-bit integer constant.
- `%rN:i32 = add.i32 %rA, %rB`, `sub.i32`, `mul.i32`, `div.i32`, and `rem.i32` perform signed 32-bit integer arithmetic.
- `%rN:i32 = and.i32 %rA, %rB`, `or.i32`, `xor.i32`, `shl.i32`, and arithmetic `shr.i32` perform bitwise 32-bit integer
  operations.
- `%rN:bool = eq.i32 %rA, %rB` and `ne.i32` compare two `i32` values for equality or inequality.
- `%rN:bool = lt.i32 %rA, %rB`, `le.i32`, `gt.i32`, and `ge.i32` perform signed `i32` comparisons.
- `%rN:i64 = add.i64 %rA, %rB`, `sub.i64`, `mul.i64`, `div.i64`, and `rem.i64` perform signed 64-bit integer arithmetic.
- `%rN:i64 = and.i64 %rA, %rB`, `or.i64`, `shl.i64`, and arithmetic `shr.i64` perform bitwise 64-bit integer operations.
- `%rN:bool = eq.i64 %rA, %rB` and `ne.i64` compare two `i64` values for equality or inequality.
- `%rN:bool = lt.i64 %rA, %rB`, `le.i64`, `gt.i64`, and `ge.i64` perform signed `i64` comparisons.
- `%rN:bool = not.bool %rA` negates a boolean SSA value.
- `%rN:type = call <symbol>(%rA, %rB)` calls another function and stores a typed result.
- `call <symbol>(%rA, %rB)` calls a void function and discards the result.
- `store %rN, %sA` writes a typed register value to a matching stack slot.
- `%rN:type = load %sA` reads a matching stack slot into a new typed register value.
- `br bbN` transfers control to another basic block.
- `br_if %rN, bbA, bbB` branches to `bbA` when the `bool` condition `%rN` is true, otherwise to `bbB`.
- `ret` and `ret %rN` are the current return terminators.
- `panic` terminates the current path. The LLVM backend lowers it to `llvm.trap` followed by `unreachable`.
- `.end` closes a function body.

A parameter-returning definition therefore has an explicit body mapping:

```text
.func Identity : (i64) -> i64
.param %r0:i64
bb0.entry:
  ret %r0
.end
```

Call targets resolve only within the registry module. Forward references are accepted, but every target must be declared by
`.extern` or `.func` and its argument and return types must match the call record.

Stack slots are function-local and target-independent. Slot ids are sequential, `void` slots are invalid, and load/store
value types must exactly match the slot type. The LLVM backend currently materializes them with entry-block `alloca`
instructions and typed `load`/`store` operations.

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

The C23 API currently includes module construction, verification, text writing, v0 text parsing, and read-only
function/body inspection. Direct `xs build --xlil -file <input.xlil>` uses this parser API before emitting verified and
optimized LLVM IR, an object file, and a native `.xse` executable for the supported local-target subset.

## Direct native entry point

Direct native builds require exactly one defined XLIL entry function:

```text
.func main : () -> i32
bb0.entry:
  %r0:i32 = const.i32 0
  ret %r0
.end
```

The direct driver uses this function as the operating-system process entry point without generating an X# runtime wrapper.
It runs the configured LLVM verification/optimization pipeline, then writes `.ll`, `.o`, and `.xse` executable artifacts next
to the input registry. The current direct XLIL path uses the backend O0 pipeline. Linking is limited to the local Linux ELF
host and does not provide runtime or external-library resolution. PE `.xse` output is planned after ELF support.
