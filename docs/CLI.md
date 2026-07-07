<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# CLI contract

The `xs` command currently works through a project manifest. The manifest syntax is the `.xsproj` format; it is not JSON,
TOML, or YAML.

## Supported forms

```text
xs check -proj MyApp.xsproj
xs build -proj MyApp.xsproj
xs run -proj MyApp.xsproj
xs build --output hir -proj MyApp.xsproj
xs build --output mir -proj MyApp.xsproj
xs build --output xlil -proj MyApp.xsproj
```

On usage errors, the CLI prints:

```text
usage: xs <check|run> -proj <project.xsproj>
usage: xs build [--output hir|mir|xlil] -proj <project.xsproj>
```

## `xs check`

`xs check` does not produce objects. Current flow:

1. `.xsproj` parse/model validation
2. source discovery
3. X# parse/structural AST
4. macro validation and expansion preparation
5. HIR symbol/import/name/type resolution
6. early expression checks

## `xs build`

`xs build` will eventually run the full check, MIR, borrow checker, monomorphization, XLIL, backend, object, and link flow.
Today, some output modes may intentionally fail because the intermediate formats are not complete yet.

## `xs run`

`xs run` will eventually run `xs build` first and then execute the generated executable. Full run semantics are not considered
ready until native executable generation is complete.

## Intermediate outputs

- `.xhir`: HIR text
- `.xmir`: MIR text
- `.xlil`: XLIL text registry

`.xlil` will never be a binary format.

## Future direct XLIL build

Planned form:

```text
xs build --xlil -file foo.xlil
```

This path will skip project manifests and run XLIL parse/verify/backend/link directly. It is not implemented yet.
