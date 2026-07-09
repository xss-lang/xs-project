<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# XHIR text format

`.xhir` is the human-readable text form of checked X# HIR. It is not assembly-like, not binary, and not an opaque compiler
serialization format.

XHIR exists for semantic inspection: names, modules, imports, declarations, resolved types, generic parameters, interfaces,
macro expansion results, and expression/type-check information. It should read like a structured semantic tree, not like an
instruction stream.

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
- XHIR must not use XLIL-style directives such as `.func`, `.extern`, or `%0:type = op`.

## Shape

The exact grammar will be tightened as HIR stabilizes, but the intended shape is record/tree oriented:

```text
.xhir version 0
module App

imports
  import std.io
.end

declarations
  function Main
    visibility public
    signature
      parameters []
      returns void
      throws []
    body
      block
        let message
          type Str
          value string "Hello from X#"
        call std.io.println
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
- literal, local, assignment, let, expression, and return records
- `analysis typecheck` records for type-check diagnostics, spans, and messages

The version header is part of the `xs build` input contract. It tells the compiler which XHIR text grammar version it is
being asked to parse. Version `0` is the only supported XHIR version today; future versions can be accepted explicitly
without changing the human-readable text requirement.

Additional declaration kinds, generic constraints, interface relations, macro-expansion records, and richer expression forms
will be added as the checked HIR model stabilizes.

## Non-goals

- XHIR is not executable input for a backend.
- XHIR is not a lossless source-code pretty-printer.
- XHIR is not the public XLIL API surface.
- XHIR is not required to preserve trivia such as comments or exact whitespace.
