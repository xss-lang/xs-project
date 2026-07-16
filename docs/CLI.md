<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# CLI contract

The command surface is split by responsibility. `/usr/bin/xs` is the JVM-free compiler, `/usr/bin/xs-project` evaluates
modern Kotlin project files on JRE 25 or newer, and `/usr/bin/xs-proj` parses and validates feature-frozen `.xsproj` files.

## Supported forms

```text
xs check
xs build
xs run
xs check -proj MyApp.xsproj
xs build -proj MyApp.xsproj
xs run -proj MyApp.xsproj
xs-proj MyApp.xsproj
xs build -file Main.xs
xs build --output hir -file Main.xs
xs build --output mir -file Main.xs
xs build --output xlil -file Main.xs
xs build --hir -file Main.xs
xs build --mir -file Main.xs
xs build --xlil -file Main.xs
xs build --warning all --werror true --verbose true
xs build --module ./Modules
xs build -proj Example.xsproj --module ./Modules
xs --version
```

The `-proj` flag is accepted only by `xs` and only for `.xsproj` input. Kotlin projects never use `-proj`; argument-free
`xs check`, `xs build`, and `xs run` ask `xs-project` to discover and evaluate `xs.project.kts` or the
`xs.settings.kts` + `xs.build.kts` pair. `xs-project` returns source metadata and never parses or compiles `.xs` files.
`xs-proj` accepts a manifest path directly and performs parser/model validation only.

`--module <directory>` supplies a recursive module source root when the KTS project does not declare
`module { include(...) }`. It is mandatory when a legacy `.xsproj` build is combined with a sibling `xs.module.kts`;
that combination is never discovered implicitly.

## One-shot compiler policy

The following options override compiler policy for one invocation and may be combined with an argument-free Kotlin
project build, `-proj`, or `-file`:

- `--warning all|medium|low|none` selects warning volume. A warning declares the minimum volume at which it is active;
  `all` enables every warning, `medium` is the default, `low` keeps only the most important warnings, and `none` disables
  warnings.
- `--werror true|false` controls whether enabled warnings fail the compilation.
- `--verbose true|false` controls compiler progress output. When enabled, `xs` prints the effective policy and the
  ordered source registry entering the frontend.

The default policy is `warning=medium`, `werror=false`, and `verbose=true`. An explicit KTS `compiler {}` value is used
for that project, and a command-line value has final precedence for the current invocation.

For Kotlin projects these values override the evaluated `compiler {}` block without modifying either KTS file:

```text
xs build --warning low --werror false --verbose true
```

XSPROJ has no persistent compiler-policy section. A legacy build uses the same options only as one-shot values:

```text
xs build -proj MyApp.xsproj --warning all --werror true
```

The compiler usage is:

```text
usage: xs build -file <Main.xs>
usage: xs <check|build|run>
       [--warning all|medium|low|none] [--werror true|false] [--verbose true|false]
usage: xs <check|build|run> -proj <project.xsproj>
usage: xs build [--output hir|mir|xlil] -file <input>
usage: xs build [--hir|--mir|--xlil] -file <input>
usage: xs --version
```

## `xs --version`

`xs --version` prints the compiler version, such as `xs 0.1.7`.

## `xs check`

`xs check` does not produce objects. Current flow:

1. source input validation
2. X# parse/structural AST
3. macro validation and expansion preparation
4. HIR symbol/import/name/type resolution
5. early expression checks

## `xs build`

`xs build` will eventually run the full check, MIR, borrow checker, monomorphization, XLIL, backend, object, and link flow.
Today, plain `xs build -file <main.xs>`, argument-free `xs build`, and `xs build -proj <App.xsproj>` can produce a native
`.xse` only for the first
supported source slice:

```xs
fn add(left: Long, right: Long) -> Long { return left + right; }
fn main() -> Long { return add(2, 5); }
```

Project builds may place supported helper functions in separate selected source files. The compiler merges their expanded
AST packets, collects all program signatures before lowering bodies, and emits one verified XLIL/LLVM module. Kotlin
project source globs are resolved before this compiler stage and always put the sole `main.xs` entry first.

The entry function must be top-level, named `main`, have no parameters, and return `Long`. Same-module helper functions may
take `Long`, `Int`, or `Bool` parameters and return `Long`, `Int`, or `Bool`. Supported bodies may contain explicit
`Long`/`Int`/`Bool` local bindings or
inferred `:=` local bindings with i32-compatible or bool-compatible initializers, simple assignments to mutable locals,
and then one return statement. A statement-level `if` may contain one simple assignment in each branch; the local may be
read after the branches merge. Supported `Long` assignments are `=`, `+=`, `-=`, `*=`, `/=`, `%=`, `&=`, `|=`, and
`^=`; a branch may contain multiple such assignments. `val`, `const`, and `static` locals can be initialized but not
reassigned. Supported conditional assignment blocks, including `else if` chains, may nest. A supported `while` has a Bool condition and one or more
supported assignment statements and `Long`/`Bool` local declarations in its body; it lowers through the same native
control-flow path. An unconditional `loop { ... }` uses that same CFG with a constant true condition. `break` and
`continue` target the innermost supported loop. Block-local bindings are unavailable
after their enclosing block; an inner scope may shadow an enclosing binding, which becomes visible again after scope exit.
The same supported
return-expression subset may be used by an early `return` inside a supported conditional or loop block. The
native slice also supports `for (name: Long = initializer; bool_condition; update)` with a required variable initializer
and an assignment, postfix `++`, or postfix `--` update; `for each` remains deferred. The
native slice also lowers statement-level `match (value)` over a supported `Long` or `Bool` selector. Arms use matching
literal patterns and must end with an `else` arm; each arm accepts the same supported block statements as `if`.
supported return expression subset is i32-range integer literals, local identifiers, direct same-module `Long` calls, unary
`-`, `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, and one top-level `if (...) { expr; } else { expr; }` expression
whose condition is a bool literal, a `Bool` local, a direct same-module `Bool` helper call, unary `!`, or an i32 comparison,
including `==`, `!=`, `<`, `<=`, `>`, and `>=`. Local storage passes through MIR places, XLIL stack slots, and LLVM
stack operations before normal LLVM optimization. The compiler lowers that source `Long` slice to the direct native
process `i32` entry ABI. General source-level function body lowering is still incomplete.

For legacy manifests, `-proj` selects only `.xsproj`; it is not a Kotlin project flag. The `--output hir|mir|xlil`
spelling and the short `--hir`, `--mir`, and
`--xlil` spelling select the same intermediate output kind. The short spelling is currently valid only with `-file`.

## `xs run`

`xs run` will eventually run `xs build` first and then execute the generated executable. Full run semantics are not considered
ready until native executable generation is complete.

## Intermediate outputs

- `.xhir`: human-readable XHIR text
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
