<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# LLVM backend infrastructure status

The LLVM backend is the `xs_backend_llvm` library, separate from the frontend. It does not accept AST or HIR directly. HIR
and MIR do not depend on LLVM, but they may use the target-independent XLIL type/data vocabulary. LLVM C API concepts such
as target triples and data layouts exist only in the backend layer. Real function body generation will be added after typed,
borrow-checked, monomorphized MIR nodes are lowered to XLIL.

LLVM is the primary backend focus today, but the HIR/MIR/XLIL design must remain portable to other backends. Target-specific
assembly, if needed, is isolated in backend/runtime layers. NASM `.asm`/`.inc` files may be used only without locking the
design to x86-64; ARM64 compatibility must be preserved.

## Implemented components

- LLVM context and target-machine lifetime management
- Explicit target triple or native target triple selection
- Target data layout generation
- Independent LLVM module per codegen unit
- Numeric X# primitive type mapping to LLVM types
- Body-less function declaration and signature lowering
- XLIL type mapping for function declarations
- Direct `.xlil` parser/model-driven `.extern`/`.func` lowering to verified and optimized LLVM IR, objects, and local
  native `.xse` executable artifacts for `.func main : () -> i32`
- Initial XLIL body lowering for parameters, constants, i32 arithmetic/bitwise/shift/comparison, i64
  arithmetic/bitwise/shift/comparison, typed stack-slot `load`/`store`, `call`, `br`, `br_if`, `panic`, `ret`, and
  `ret %rN`
- LLVM optimization pipeline selection from `default<O0>` through `default<O3>`
- LLVM module verification
- Object file emission per codegen unit
- Linker invocation layer that does not use a shell and receives arguments from the caller

Object file emission and linker invocation are wired into direct `.xlil` native builds. Project-based `xs build` still
waits for the full frontend/HIR/MIR path to produce complete XLIL.

## Intentionally deferred type mappings

`Str` is not lowered to an LLVM storage type yet:

- `Str` is UTF-16. The compiler/runtime selects UTF-16LE or UTF-16BE automatically for the target/runtime situation.
- `Str` length is unbounded except by the representation allowed by UTF-16/runtime/platform limits.
- Semantically, `Str` corresponds to Rust's `&'static str`.
- `Optional<Str>` corresponds to Rust's `Option<String>` at the language model level, so it will need an owned string
  payload representation when Optional/String lowering reaches the backend.
- Ownership, length-field type, and runtime value layout are not fully implemented in code yet.

`Bool` is resolved as a 1-bit primitive in HIR and lowered to `i1` by the LLVM backend.

`Byte` and `SByte` are separate unsigned/signed 8-bit primitive types at the HIR level. Their LLVM storage type is `i8` for
both; signedness is selected later by typed MIR operations such as comparisons, conversions, and arithmetic.

`Char` lowers to LLVM `i16` because its documented width is 16 bits.

For `Str`, the backend returns `XS_BACKEND_DEFERRED`. This avoids inventing a temporary ABI or runtime layout.

## Preserved stage boundary

The backend order is preserved as:

```text
Borrow-checked and optimized MIR
    → monomorphization
    → codegen unit splitting
    → XLIL
    → LLVM module and function signature lowering
    → XLIL function body lowering
    → LLVM optimization
    → object file emission
    → linker invocation
```

XLIL function body lowering currently covers explicit body parameters, `i64`, `i32`, and boolean constants, i32/i64
arithmetic/bitwise/shift/comparison instructions, direct calls, unconditional `br`, conditional `br_if`, `panic`, `ret`,
typed stack slots with `load`/`store`, and `ret %rN`. Stack slots are allocated in the LLVM entry block and remain eligible
for normal LLVM promotion and scalar optimization. The current source-native bridge uses this path for `Long` and `Bool`
local initialization, reads, and simple mutable reassignment. `panic` emits an `llvm.trap` call followed by LLVM
`unreachable`.

The source-native bridge lowers supported `if`, `while`, classic `for`, and statement-level `match` control flow into MIR
branches before XLIL and LLVM lowering. A supported `match` over `Long` or `Bool` becomes ordered literal tests and branch
blocks with a required final `else` arm. This remains a narrow source slice; `for each`, general expressions, and arbitrary
runtime-backed statements are not implemented yet.
Parameter values are read from the declared
LLVM function; calls use declarations emitted for the same XLIL registry module. The backend emits declarations from the
public C API and direct `.xlil` files after they are parsed into the XLIL C model, can write verified LLVM IR text for the
current codegen unit, runs the configured LLVM verification/optimization pipeline, emits an object through LLVM, and invokes
the configured Clang driver with LLD for a native-host direct XLIL `.xse` executable. Direct XLIL currently uses the O0 pipeline;
no CLI optimization flag is exposed yet. It rejects unsupported body forms instead of inventing semantics. This prevents AST
or unfinished HIR behavior from being lowered directly to LLVM IR. Cross-target direct builds stop after object emission, and
runtime or external library resolution is not configured yet.
