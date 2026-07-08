<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# X# v0 decisions and implementation tracking list

This file is no longer a list of “large decisions to ask about.” The decisions here are the contract to implement for X# v0.
Remaining gaps are implementation gaps, not missing-decision gaps. New decisions are fixed here without waiting for more
external documentation; existing documented X# rules always take priority. Missing areas may draw on Rust, TypeScript, C#,
C23, and target assembly conventions as long as the result fits X#.

## Language semantic decisions

- Declaration conflicts after macro expansion are resolved on the AST. Same names in the same namespace and same declaration
  kind are errors. `public`/file-local visibility does not change conflict rules; visibility only controls access.
- Macro expansion is deterministic: file order, source order, and expansion-depth order are used. Infinite recursion produces
  a diagnostic at a fixed depth limit.
- Lifetime rules are taken from the Rust model: lexical region inference, outlives bounds, higher-ranked lifetime bounds, and
  elision rules are compatible with Rust 2021. When X# syntax differs, it lowers to equivalent semantic AST fields.
- X# uses nominal typing. User-defined type identity is based on names/symbols; two distinct types with the same fields and
  shape are not compatible.
- Trait/interface compatibility is nominal. A type is compatible with an interface only when it explicitly implements that
  interface. Structural compatibility may be added only if a separate `structural` feature is documented.
- Generic constraint validation is based on nominal interface membership. All constraints are conjunctive; instantiation is an
  error if any constraint is not satisfied.
- Overload selection order: namespace/import visibility filter, arity filter, exact type match, coercion-free generic
  substitution, constraint validation, most-specific candidate selection. Equal specificity produces an ambiguity diagnostic.
- Method merge follows nominal override rules along the class inheritance chain. The base declaration must be visible for an
  override with the same signature; overload sets remain separate.
- Operator resolution tries the primitive built-in operator set first, then the visible user-defined operator set. A
  user-defined operator cannot shadow a primitive exact match.
- `throws` is part of the function type. Checked exception lists are validated during HIR type checking and are represented as
  normal and exceptional edges in MIR terminators.
- `async fn` publicly returns `Task<T>`; `T` is the return type, and `Task<void>` is used for `void`. `await` is valid only on
  values satisfying the awaitable trait/interface contract.
- `Send` and `Sync` are auto-traits. Primitive immutable values are considered Send+Sync; raw runtime/resource handle types
  require explicit implementation. Mutable aliasing is prevented by the borrow checker.
- Mutability is place-based. A `mut` binding grants reassignment and mutable-borrow creation authority; an immutable binding
  cannot produce a mutable borrow.
- Drop order is reverse declaration order within the lexical scope. For partial initialization, only initialized fields/values
  are dropped.

## Runtime and ABI decisions

- `char` is a 16-bit UTF-16 code unit. It may hold a standalone surrogate code unit; `char` does not guarantee scalar values.
- `str` is a UTF-16 code-unit sequence. Length is a `size_t` code-unit count, limited by the platform address space and
  `size_t`. A null terminator is not required.
- `str` v0 runtime layout is a fat handle with `length: size_t`, `data: const uint16_t *`, and `owner: void *`.
  `owner == NULL` means a borrowed/static string.
- String literal normalization performs only escape resolution; it does not perform Unicode canonical normalization. Invalid
  escapes produce diagnostics; raw UTF-16 surrogate pairs are preserved.
- `data` and `class` field layout follows declaration order. Alignment and padding are determined by the target platform ABI.
- `class` values use reference semantics; object handles are pointer-sized opaque references.
- `data` values use value semantics; fields are stored inline.
- The default `enum` tag type is `u32`. `enum data` layout is `u32 tag` plus an aligned union large enough for the largest
  payload.
- Niche optimization does not exist in v0. ABI stability keeps enum/data-enum representation tied to explicit layout.
- Interface dispatch in v0 uses an itable: object header to type descriptor, type descriptor from interface id to method
  table.
- Generic interface dispatch happens through a concrete itable after monomorphization.
- Cross-module public interface visibility is checked through module registry and package metadata; private interface
  implementation details are not exported to other modules.
- Exception ABI in v0 uses zero-cost unwinding when platform unwind support exists; otherwise it uses a runtime personality
  abstraction. MIR cleanup edges carry drop points.
- A `throw` payload is a runtime exception object reference. `catch` performs nominal type matching.
- Async runtime ABI contains a `Task<T>` header, state enum, poll function pointer, result storage, and exception slot.
- The v0 scheduler does not require work-stealing; a platform thread pool abstraction is enough. Cancellation uses a
  cooperative flag.
- Thread/sync runtime uses C23 atomics and thin abstractions over platform-native thread primitives; it does not depend on GNU
  pthread extensions.

## MIR decisions

- A MIR function consists of a local table, a basic block list, and a mandatory terminator.
- A basic block is a single-entry label; exactly one terminator follows the final instruction.
- The value model uses typed SSA value ids. Mutable memory uses places/projections.
- A place is a chain of `local`, `field`, `deref`, and `index` projections.
- Terminator set for v0: `return`, `goto`, `switch`, `call`, `throw`, `unreachable`, `drop_then`.
- Exception paths are stored as explicit exceptional targets on terminators.
- Borrow checker inputs consist of region ids, loan ids, move paths, initialized paths, drop flags, and alias sets.
- Borrow checker rules are close to Rust NLL: shared borrows may be multiple, mutable borrows are exclusive, and moved places
  cannot be read.
- Drop-point validation runs on MIR; moved or uninitialized values are not dropped.
- MIR optimization pass order: CFG cleanup, unreachable elimination, copy propagation, constant folding, dead store
  elimination, drop flag simplification, second CFG cleanup.
- MIR optimizations must not change observable drop order, exception edges, or borrow-check results.

## Monomorphization and codegen decisions

- Monomorphization is lazy: reachable generic instantiations are generated as they are discovered.
- Cache key fields are `package-id`, `symbol-id`, canonical type arguments, target triple, and backend ABI version.
- Symbol naming uses stable mangling: `_XS` prefix, module path, symbol kind, and generic argument hash.
- The same instantiation is shared within a package boundary; public generic instantiations may be written to a cross-package
  cache.
- Codegen unit splitting is module-path based by default. Large modules may be split into sub-units by function-count
  threshold.
- The incremental cache is a content-addressed artifact store; the hash input is the canonical HIR/MIR dump, target triple,
  and compiler version.

## XLIL decisions

- XLIL is the shared public input language for backends; HIR/MIR remain tied to the XLIL type/data vocabulary instead of LLVM.
- `.xlil` is always a text registry format; no binary XLIL format will be added.
- XLIL is not as high-level as CLR and not as low-level as assembly; it is an assembly-like, target-independent
  mid/low-level registry language.
- An XLIL registry file starts with `.xlil version 0`, followed by `.xlil module <name>`, and contains directive-style
  declaration/definition records.
- Current XLIL text uses assembly-like records: `.extern`, `.func`, `bbN.label:`, `%N:type = ...`, `br bbN`, `ret`, and
  `.end`.
- The function body model uses typed SSA, explicit basic blocks, and terminators.
- XLIL instruction set v0: constants, arithmetic, compare, load/store, address projection, call, branch, switch, return,
  throw, landingpad/cleanup, aggregate construct/extract.
- XLIL does not describe runtime ABI-specific object layout; it only carries target-independent types, symbols, and control
  flow.
- MIR → XLIL lowering runs after borrow checking and monomorphization.
- XLIL → LLVM IR lowering intentionally uses target triple/data layout to produce LLVM modules, functions, and bodies.
- The XLIL public C23 API under `#include <xs/lil.h>` provides AOT generation, XLIL registry generation, module, type,
  function, block, value, and instruction builder surfaces.
- Third-party languages can generate XLIL through `xs/lil.h` and target the LLVM backend and future XS Backend.

## JIT and public intermediate API decisions

- HIR baseline JIT public C23 API target: `#include <xs/hir/jit.h>`.
- HIR baseline JIT is for fast development, checking, and debug execution; it is not the optimized native performance target.
- MIR performance JIT public C23 API target: `#include <xs/mir/jit.h>`.
- MIR performance JIT runs on typed, borrow-checked, optimized MIR.
- XLIL AOT public C23 API target: `#include <xs/lil/aot.h>`.
- JIT/AOT APIs do not tie HIR/MIR to the LLVM API; when needed, they lower through XLIL or a backend abstraction.
- These API surfaces are designed as public C23 ABI for third-party languages and tools.
- JIT headers and behavior will not be extended with fake/stub semantics before implementation exists.

## Backend and link decisions

- LLVM backend v0 is the primary backend. The XS Backend option is accepted at parse/model level; until implementation is
  ready, it produces a feature diagnostic.
- Object output name is `<target-name>.o` or the platform object suffix. Executable output is derived from the project
  `appName`.
- Linker abstraction does not invoke a shell; it runs through an argv list.
- On Linux, the default linker path is `ld.lld` or `clang --target=<triple> -fuse-ld=lld`; GNU binutils are not used.
- Runtime linking is done through an explicit XS runtime archive/shared object.
- Object/link artifact directory is `build/xs/<profile>/<target>/`.

## Tooling and project-flow decisions

- `.xhir`, `.xmir`, and `.xlil` are human-readable text formats. They are deterministic, newline-normalized, and
  source-order stable.
- `.xhir` and `.xmir` are not binary formats and are not opaque serialized compiler state. They must stay suitable for
  direct inspection, code review, and text-based regression fixtures.
- `.xhir` and `.xmir` are not assembly-like. XHIR is a structured semantic tree/record dump; XMIR is a structured
  control-flow/borrow-analysis dump.
- `xsfmt`, `xstidy`, and future developer tool projects use TOML for user configuration.
- TOML is only the tool configuration standard; it does not replace the `.xsproj` project manifest format.
- `xs check` runs parse, macro expansion, HIR, type-check, and borrow-independent semantic checks; it does not produce objects.
- `xs build` runs all check stages, MIR, borrow checker, monomorphization, XLIL, backend, and link flow.
- `xs run` first runs `xs build`, then launches the executable with argument forwarding.
- `xs build --output hir|mir|xlil -file <input>` and `xs build --hir|--mir|--xlil -file <input>` run direct file flows
  without reading a project manifest.
- `xs build --xlil -file <input.xlil>` parses/verifies XLIL, then runs backend and link.
- Successful commands exit with code `0`; diagnostic errors exit with `1`; internal compiler errors exit with `70`.
- Diagnostic codes use the `XS####` format. Severity values: note, warning, error, fatal.
- Machine-readable diagnostic output will be provided later as JSON Lines through `--diagnostic-format json`.
- `compilerOptions.xsBackend: "LLVM"` selects the LLVM backend; `"XS"` produces a feature diagnostic until XS Backend is
  ready.
- Public module/package distribution includes manifest metadata, exported symbol table, and compiled interface summary.
- Cross-project import resolution uses package name/version, target triple compatibility, and exported module path.

## Implementation status

- The decisions above are fixed; implementation will move toward this contract incrementally.
- No temporary or undocumented ABI/API will be added. New implementation work will be tied to this file and to the
  [IMPLEMENTATION.md](IMPLEMENTATION.md) flow.
