# Compiler architecture

The X# compiler is developed as a staged, testable pipeline. Each stage produces its own data model; later stages must not
guess missing semantics inside the backend.

## Pipeline

```text
.xs sources
    → lexer
    → parser
    → structural AST
    → macro validation and expansion view
    → HIR symbol/import/name/type resolution
    → expression/type checks
    → MIR
    → borrow checker
    → MIR optimization
    → monomorphization
    → codegen unit planning
    → XLIL
    → LLVM IR or future XS Backend lowering
    → object emission
    → link
```

## Layer boundaries

### Frontend

The frontend owns `.xs` source text, tokens, parsing, and the structural AST. The parser produces syntax; it must not invent
LLVM or MIR behavior for unfinished semantics.

### Macro layer

The macro system currently provides validation, token expansion, synthetic reparse, and expanded views. In-place AST
replacement is not complete yet. HIR consumers read macro-call replacements through expanded-view APIs.

### HIR

HIR is responsible for:

- module/namespace/import resolution
- symbol table generation
- visibility checks
- user-defined and primitive type resolution
- generic arity and constraint resolution
- early expression/mutability diagnostics

HIR does not depend on the LLVM API. Type information remains target-independent and may reference the XLIL type vocabulary
when needed.

### MIR

MIR is the control-flow, local/place/value, and terminator model. The borrow checker and MIR optimizer operate on this model.
MIR does not yet provide complete statement/expression lowering or exception/async state-machine generation.

### XLIL

XLIL is the shared input language for backends. `.xlil` is a text registry format and will not be binary. Third-party
frontends will eventually be able to generate XLIL through the `xs/lil.h` public C23 API.

### Backend

The LLVM backend is separate from the frontend. LLVM context, target machine, module, data layout, function declaration
lowering, optimization pipeline, object emission, and linker invocation live here. HIR/MIR/XLIL headers do not expose LLVM C
API concepts.

## Major unfinished pieces

- Full AST macro replacement
- General expression type inference and overload resolution
- Nominal interface membership validation
- Send/Sync, async/await, and checked-exception type checking
- MIR statement/expression lowering
- Full borrow-checker region/loan/drop model
- Monomorphization and incremental cache
- XLIL → LLVM function body lowering
- End-to-end `xs build` / `xs run` executable generation

These gaps are tracked in [IMPLEMENTATION.md](IMPLEMENTATION.md) and [TODO.md](TODO.md).
