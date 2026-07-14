<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# XHIR text format

`.xhir` is the human-readable text form of XHIR, the normalized operational side of X# HIR. It is not assembly-like, not
binary, and not an opaque compiler serialization format.

HIR is split into coordinated THIR and XHIR sides. THIR carries the typed, source-oriented semantic structure; XHIR carries
the normalized high-level operations used by MIR lowering. These are parts of one HIR layer, not unrelated pipeline IRs.
A `.xhir` document represents XHIR only and is not a textual THIR dump.

XHIR exists for MIR input and semantic inspection: names, modules, imports, declarations, resolved types, generic
parameters, interfaces, macro expansion results, and expression/type-check information. It should read like a structured
semantic tree, not like an instruction stream.

## Design rules

- The format is deterministic, newline-normalized, UTF-8 text.
- Source-order is preserved unless a section explicitly says it is sorted.
- The top level is a named document with nested records.
- Records use stable field names instead of positional operands.
- Indentation is for readability only; parsers must use explicit records such as `.end` and `.program end`, not leading
  spaces, to determine structure.
- Identifiers, paths, and types are printed in their X# spelling where that spelling is still meaningful.
- Compiler-generated ids are allowed only when they help diagnostics or cross-reference repeated records.
- XHIR must remain suitable for code review and text fixtures.
- XHIR must not use XLIL-style directives such as `.func`, `.extern`, or `%r0:type = op`.

## Shape

The exact grammar will be tightened as HIR stabilizes, but the intended shape is record/tree oriented:

```text
.xhir version 0
module App

imports
  import std::io
.end

declarations
  function Main
    visibility public
    signature
      parameters []
      returns ()
    body
      block
        let message
          type Str
          value string "Hello from X#"
        call std::io.println
          arguments [message]
.end
.program end
```

This is intentionally closer to a semantic document than to assembly. A formatter should be able to align indentation, and a
reviewer should be able to understand which declaration or expression changed without reconstructing a low-level CFG.

## Current parser subset

Rust `xslang` currently parses the module-symbol and checked-function subsets emitted by the first XHIR writer:

- `.xhir version 0`
- `module <name>` with `imports`, `declarations`, and `symbol` records
- `function <name>` with `signature`, `locals`, and `body`
- explicit `.end` section markers and `.program end` document marker
- primitive and named type records
- literal, local, assignment, prefix/postfix update, typed direct-call, conditional block, loop, classic-for,
  statement-match, `propagate`, let, expression, and return records
- `analysis typecheck` records for type-check diagnostics, spans, and messages

The version header is part of the `xs build` input contract. It tells the compiler which XHIR text grammar version it is
being asked to parse. Version `0` is the only supported XHIR version today; future versions can be accepted explicitly
without changing the human-readable text requirement.

Additional declaration kinds, generic constraints, interface relations, macro-expansion records, and richer expression forms
will be added as the checked HIR model stabilizes.

A typed direct call is represented as a semantic record rather than an instruction:

```text
call add : (Long, Long) -> Long
  argument
    literal integer 3
  argument
    literal integer 4
```

Parameter types are part of checked HIR. Commas inside nested generic types do not split the outer parameter list.
Unit is written as `()` in call signatures. A call used as a semicolon-terminated statement remains a call record; MIR
decides whether it has no result register (`-> ()`) or evaluates and discards a typed value result.

Increment and decrement preserve whether the source expression returns the old or updated binding value:

```text
update postfix increment value
update prefix decrement value
```

MIR lowering turns these semantic records into explicit load, integer operation, store, and selected result-value steps.

Statement match records retain the checked selector type and semantic arms. A final `else` arm is required:

```text
match Long
  selector
    local value
  arm literal integer 2
    body
      return
        literal integer 7
    .end
  arm else
    body
      return
        literal integer 3
    .end
.end
```

Conditional expressions retain semantic blocks and their checked result type:

```text
if_expression Long
  condition
    local enabled
  then
    tail
      literal integer 1
  .end
  else
    tail
      literal integer 2
  .end
```

The `.end` records, rather than indentation, delimit each branch.

Classic `for` records retain the semantic header positions instead of encoding them through indentation:

```text
for
  initializer
    let index
      type Long
      mutability mutable
      initializer
        literal integer 0
  condition
    binary lt
      left
        local index
      right
        literal integer 3
  update
    assign index
      binary add
        left
          local index
        right
          literal integer 1
  body
    expression
      local index
  .end
.end
```

Any omitted initializer, condition, or update section is absent from the record. The body and final `.end` remain required.

## Non-goals

- XHIR is not executable input for a backend.
- XHIR is not THIR, and `.xhir` is not a THIR serialization.
- XHIR is not a lossless source-code pretty-printer.
- XHIR is not the public XLIL API surface.
- XHIR is not required to preserve trivia such as comments or exact whitespace.
