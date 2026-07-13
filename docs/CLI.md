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
xs build -file Main.xs
xs build --output hir -proj MyApp.xsproj
xs build --output mir -proj MyApp.xsproj
xs build --output xlil -proj MyApp.xsproj
xs build --output hir -file Main.xs
xs build --output mir -file Main.xs
xs build --output xlil -file Main.xs
xs build --hir -file Main.xs
xs build --mir -file Main.xs
xs build --xlil -file Main.xs
xs --version
```

On usage errors, the CLI prints:

```text
usage: xs <check|run> -proj <project.xsproj>
usage: xs build -file <Main.xs>
usage: xs build [--output hir|mir|xlil] -proj <project.xsproj>
usage: xs build [--output hir|mir|xlil] -file <input>
usage: xs build [--hir|--mir|--xlil] -file <input>
usage: xs --version
```

## `xs --version`

`xs --version` prints the compiler version, such as `xs 0.0.8`.

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
Today, plain `xs build -file <Main.xs>` and `xs build -proj <App.xsproj>` can produce a native `.xse` only for the first
supported source slice:

```xs
fn Add(left: Long, right: Long) -> Long { return left + right; }
fn main() -> Long { return Add(2, 5); }
```

The entry function must be top-level, named `main`, have no parameters, and return `Long`. Same-module helper functions may
take `Long` parameters and return `Long` or `Bool`. Supported bodies may contain explicit `Long`/`Bool` local bindings or
inferred `:=` local bindings with i32-compatible or bool-compatible initializers, simple assignments to mutable locals,
and then one return statement. A statement-level `if` may contain one simple assignment in each branch; the local may be
read after the branches merge. Supported `Long` assignments are `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, and
`^=`; a branch may contain multiple such assignments. `val`, `const`, and `static` locals can be initialized but not
reassigned. Supported conditional assignment blocks, including `else if` chains, may nest. A supported `while` has a Bool condition and one or more
supported assignment statements and `Long`/`Bool` local declarations in its body; it lowers through the same native
control-flow path. `break` and `continue` target the innermost supported loop. Block-local bindings are unavailable
after their enclosing block, and same-name shadowing remains rejected in this compiler slice. The same supported
return-expression subset may be used by an early `return` inside a supported conditional or loop block. The
native slice also supports `for (name: Long = initializer; bool_condition; name += value)` with a required variable
initializer and assignment update; `for each` remains deferred. The
supported return expression subset is i32-range integer literals, local identifiers, direct same-module `Long` calls, unary
`-`, `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, and one top-level `if (...) { expr; } else { expr; }` expression
whose condition is a bool literal, a `Bool` local, a direct same-module `Bool` helper call, unary `!`, or an i32 comparison,
including `==`, `!=`, `<`, `<=`, `>`, and `>=`. Local storage passes through MIR places, XLIL stack slots, and LLVM
stack operations before normal LLVM optimization. The compiler lowers that source `Long` slice to the direct native
process `i32` entry ABI. General source-level function body lowering is still incomplete.

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
`.xlil` is a human-readable backend input registry. They will not be binary formats or opaque serialized compiler state.
Future `.xhir` and `.xmir` grammar changes must preserve direct human inspection and code-review friendliness. XHIR and
XMIR are not assembly-like formats; XHIR is a structured semantic tree/record dump, while XMIR is a structured
control-flow/analysis dump.

## Direct file builds

Recognized forms:

```text
xs build --output hir -file foo.xs
xs build --output mir -file foo.xs
xs build --output xlil -file foo.xs
xs build -file foo.xs
xs build --hir -file foo.xs
xs build --mir -file foo.xs
xs build --xlil -file foo.xlil
```

The direct file paths skip project manifests. Their final semantics depend on the selected intermediate kind:

- `hir` with `.xs`: parse/check a single `.xs` input and emit `.xhir`.
- `hir` with `.xhir`: parse the `.xhir version N` header and accept only supported XHIR grammar versions.
- `mir` with `.xs`: parse/check/lower a single `.xs` input and emit `.xmir`.
- `mir` with `.xmir`: parse the `.xmir version N` header and accept only supported XMIR grammar versions.
- `xlil` with `.xs`: lower a single X# source file to `.xlil`.
- `xlil` with `.xlil`: parse and verify the `.xlil version N` registry, lower the supported subset to LLVM IR, run the
  configured LLVM verification/optimization pipeline, emit an object file, and link a native executable when the target is
  the local host.
- no output flag with `.xs`: check the source file and, for the first supported `main` slice, emit `.ll`, `.o`, and
  `.xse` beside the input.

The CLI recognizes the forms now; full production semantics are still being connected.
For direct `.xhir`, `.xmir`, and `.xlil` inputs, the current CLI already validates the leading version header and rejects
unsupported grammar versions. A supported direct `.xlil` input is parsed through the public XLIL C23 parser API, verified,
lowered through the LLVM backend, verified by LLVM, and passed through the configured optimization pipeline. It writes
`<input-stem>.ll`, `<input-stem>.o`, and a native executable named `<input-stem>.xse` alongside the input file. The direct
XLIL path currently uses the backend default `O0` pipeline; no CLI optimization flag is exposed yet.

Direct native XLIL builds require exactly one function definition with this platform entry signature:

```text
.func main : () -> i32
```

This is a direct XLIL build ABI, not the X# source-language entry-point rule. The command uses the Clang driver with LLD
for local Linux ELF linking. When the configured target triple differs from the host, it still writes the LLVM IR and object
file but stops before linking with a cross-linking diagnostic. Runtime and external-library linking are not configured yet;
an unresolved XLIL `.extern` symbol causes the linker to report a failed direct native build.

The `.xse` extension is the xs-project native executable artifact name. The first target format is Linux ELF; PE native
executables are planned after ELF support.
