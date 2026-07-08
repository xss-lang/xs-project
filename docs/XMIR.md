<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# XMIR text format

`.xmir` is the human-readable text form of X# MIR. It is not assembly-like, not binary, and not an opaque compiler
serialization format.

XMIR exists for ownership, borrow-checking, drop planning, exception edges, async lowering, and optimization inspection. MIR
is lower than HIR, so XMIR may expose blocks, places, temporaries, and terminators, but it should still read as a structured
control-flow document rather than an assembly listing.

## Design rules

- The format is deterministic, newline-normalized, UTF-8 text.
- Function bodies are grouped by function, then by named control-flow sections.
- Blocks are records with explicit fields: parameters, statements, terminator, successors, and optional analysis notes.
- Statements use descriptive names and named fields rather than XLIL-style typed SSA assignment syntax.
- Places, projections, moves, borrows, drops, and regions are printed in stable symbolic form.
- Borrow-checker and optimizer annotations are allowed when they are explicitly marked as analysis output.
- XMIR must remain suitable for code review and text fixtures.
- XMIR must not use XLIL-style directives such as `.func`, `.extern`, or `%0:type = op`.

## Shape

The exact grammar will be tightened as MIR stabilizes, but the intended shape is CFG/document oriented:

```text
.xmir version 0
function Main
returns void

parameters
  parameter value
    type i64

locals
  local 0
    name value
    type i64
    mutability immutable

control_flow
  block 0
    statements
      statement use
        local 0
    terminator return

analysis verify
  diagnostic missing_terminator
    span 1:0..1
    message MIR block is missing a terminator
```

XMIR can expose control-flow because that is MIR's job, but it should do so with named records and analysis sections. XLIL is
the later assembly-like backend input; XMIR is the compiler's readable mid-level reasoning dump.

## Current parser subset

Rust `xslang` currently parses the function/control-flow subset emitted by the first XMIR writer:

- `.xmir version 0`
- `function <name>`
- `returns <xlil-type>`
- `parameters` with `parameter <name>` and `type <xlil-type>` records
- `locals`, including optional `type <xlil-type>` records for values that can lower into XLIL
- `control_flow`
- `block`
- local-use, move, borrow, end-borrow, `const.i64`, `const.bool`, arithmetic, compare, `call`, and drop statements
- `return`, `goto`, `branch_if`, `unreachable`, and `missing` terminators
- optional local return values, goto targets, and named `branch_if` condition/then/else fields
- `analysis optimizer` records for optimization pass reports and removed item counts
- `analysis verify` records for structural verifier diagnostics, spans, and messages

Parsed XMIR can be passed to the Rust MIR structural verifier. The current verifier checks duplicate local/block ids,
missing terminators, unknown local references, and unknown block targets before borrow checking.
The verified optimizer API uses the same structural verifier before and after MIR optimization.

Place projections, exception edges, drop trees, borrow regions, and optimizer annotations will be added as MIR grows.

## Non-goals

- XMIR is not the backend input language.
- XMIR is not bytecode.
- XMIR is not required to be accepted by `xs build --xlil`.
- XMIR is not a replacement for the public `xs/mir/jit.h` API.
