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
xs build --output hir -file Main.xs
xs build --output mir -file Main.xs
xs build --output xlil -file Main.xs
xs build --hir -file Main.xs
xs build --mir -file Main.xs
xs build --xlil -file Main.xs
```

On usage errors, the CLI prints:

```text
usage: xs <check|run> -proj <project.xsproj>
usage: xs build [--output hir|mir|xlil] -proj <project.xsproj>
usage: xs build [--output hir|mir|xlil] -file <input>
usage: xs build [--hir|--mir|--xlil] -file <input>
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

`-proj` and `-file` are mutually exclusive. The `--output hir|mir|xlil` spelling and the short `--hir`, `--mir`, and
`--xlil` spelling select the same intermediate output kind. The short spelling is currently valid only with `-file`.

## `xs run`

`xs run` will eventually run `xs build` first and then execute the generated executable. Full run semantics are not considered
ready until native executable generation is complete.

## Intermediate outputs

- `.xhir`: human-readable HIR text
- `.xmir`: human-readable MIR text
- `.xlil`: XLIL text registry

`.xhir`, `.xmir`, and `.xlil` are text formats. `.xhir` and `.xmir` are human-readable compiler intermediate dumps;
`.xlil` is a human-readable backend input registry. They will not be binary formats.

## Direct file builds

Recognized forms:

```text
xs build --output hir -file foo.xs
xs build --output mir -file foo.xs
xs build --output xlil -file foo.xs
xs build --hir -file foo.xs
xs build --mir -file foo.xs
xs build --xlil -file foo.xlil
```

The direct file paths skip project manifests. Their final semantics depend on the selected intermediate kind:

- `hir`: parse/check a single `.xs` input and emit `.xhir`.
- `mir`: parse/check/lower a single `.xs` input and emit `.xmir`.
- `xlil` with `.xs`: lower a single X# source file to `.xlil`.
- `xlil` with `.xlil`: parse/verify/backend/link an existing XLIL registry.

The CLI recognizes the forms now; full production semantics are still being connected.
