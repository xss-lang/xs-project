<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Current X# compiler status

The compiler is written in C23 and uses Clang, CMake, Ninja, LLVM tools, and LLD. New and touched C code uses C23
`bool` directly, does not include `<stdbool.h>`, and prefers `nullptr` over `NULL`.

The documented compilation order is preserved:

```text
.xs sources
    → lexing and parsing
    → structural AST
    → macro expansion
    → HIR and dependency graph
    → type checking
    → MIR
    → borrow checker
    → MIR optimization
    → monomorphization
    → codegen unit splitting
    → XLIL
    → LLVM IR and LLVM optimization
    → object code
    → linking
```

## Implemented infrastructure

### Monorepo layout

- The repository has moved to an LLVM-project-like monorepo selection model.
- The monorepo root is `xs-project`; the top-level CMake project name is `xs_project`.
- Top-level CMake recognizes `XS_ENABLE_PROJECTS`; the default stable project is `xs`.
- `XS_ENABLE_RUNTIMES` was added in parallel with LLVM’s `LLVM_ENABLE_RUNTIMES`; there is no buildable runtime today.
- `XS_ENABLE_PROJECTS=all` selects all stable projects.
- `xs` and `xsproj` are stable buildable projects.
- The root `include/` directory is reserved for shared public C headers across projects; `xs/include/` is for `xs` only, and
  `xsproj/include/` is for `xsproj` only.
- `xsfmt` and `xstidy` are future Rust nightly + Serde projects; `xs-analyzer` is a future Rust language server and
  TypeScript VS Code extension project; `xslang` is the future Rust compiler-core crate; `xs-backend` is a future native
  backend project; `xsrt` is a future runtime. They are not included in the CMake build yet.

### XLIL-bound middle-layer rule

- HIR and MIR do not and will not depend on LLVM.
- HIR and MIR are tied to the target-independent XLIL type/data vocabulary.
- HIR/MIR headers do not contain LLVM C API, target triple, data layout, or LLVM type/value concepts. They may include XLIL
  headers.
- XLIL is positioned before LLVM IR as the shared low-level vocabulary for HIR/MIR and the common input for backends.
- LLVM IR generation is a separate backend layer; HIR/MIR/XLIL do not carry LLVM C API concepts.
- Backend consumers lower typed, borrow-checked, monomorphized MIR to XLIL and then to their own target IR.
- Planned public API surfaces are `#include <xs/hir/jit.h>` for the HIR baseline JIT, `#include <xs/mir/jit.h>` for the MIR
  performance JIT, and `#include <xs/lil/aot.h>` for XLIL AOT. These APIs must not make HIR/MIR depend on the LLVM C API.
- LLVM is the current primary backend focus, but the architecture remains open to Cranelift, a C backend, an interpreter, or
  other targets.
- Target-specific assembly, if needed, belongs in a separate backend/runtime layer. NASM `.asm`/`.inc` use is allowed, but it
  must not lock the design to x86-64; ARM64 compatibility must be preserved.

### Supported build toolchain

- CMake is configured and tested with Clang, Ninja, LLVM tool equivalents, and LLD.
- Archive, symbol, object-copy, object-dump, strip, and linker tools are selected from the LLVM tool family.
- Other compiler or generator combinations are outside the supported build contract.
- The compiler is built in strict ISO C23 mode with `-std=c23` and `CMAKE_C_EXTENSIONS OFF`.
- The build uses strict ISO C23 rather than compiler-extension dialects.

### Project system

- `.xsproj` files are parsed with the X# project manifest syntax.
- The `.xsproj` lexer, parser, and project model implementation live under `xsproj/sources/`.
- `.xsproj` lexer/parser code is not shared with the `.xs` lexer/parser. It supports only `//` and `///` line comments; it
  does not support multiline comments.
- The `.xsproj` parser is not just an internal compiler detail. A public C23 API surface under `#include <xs/project.h>` lets
  third-party tools read `.xsproj` files in a JSON-like model.
- `externalModules.addModule` records include `moduleName`, `moduleRepo`, and `moduleVersion`. They are parsed and stored as
  manifest metadata; repository fetching and external module resolution are future work.
- Required fields, duplicate fields, unknown fields, and `appRelease` values are validated.
- When `entry: None`, the documented first additional source selection rule is applied.
- Project-relative paths are resolved from the directory containing the `.xsproj` file.
- `xs check -proj <project.xsproj>` works.
- `include!` is the built-in source-inclusion macro. It runs after the enclosing source first has a structural AST, then
  reparses the included local source at the call site; it is not a lexer/preprocessor step or a `macro_rules!` declaration.
- `xs build --output hir|mir|xlil -proj <project.xsproj>` options are recognized.
- `xs build --output hir|mir|xlil -file <input>` and `xs build --hir|--mir|--xlil -file <input>` are recognized.
- Direct `.xhir` and `.xmir` inputs currently validate only their version headers.
- Direct `.xlil` inputs are parsed and verified through the public XLIL C23 parser API. A supported local-target native
  input runs through LLVM lowering, module verification, the configured optimization pipeline, object emission, and the
  Clang/LLD `.xse` executable path.
- Plain source native builds support the first expression/local/call slice for top-level `fn main() -> Long` plus
  same-module helper functions shaped as `fn Name(<Long params>) -> Long` or `fn Name(<Long params>) -> Bool`: explicit
  `Long`/`Bool` local bindings, inferred `:=` bindings with i32-compatible or bool-compatible initializers, local
  identifier returns, direct helper calls,
  i32-range integer literals, unary `+`/`-`, `+`, `-`, `*`, `/`, `%`, `&`, `|`, `^`, `<<`, `>>`, and one top-level `if`
  expression with a bool literal, `Bool` local, direct same-module `Bool` helper call, unary `!`, or i32 comparison
  condition. `!=` and unary `!` can either swap branch targets for condition-only control flow or materialize as a `not.bool` value for supported `Bool` local
  initializers. Syntactically constant conditions, such as `false`, `!true`, or i32 literal comparisons like `1 < 2`, lower
  only the selected branch in this slice. This lowers through C MIR, XLIL, LLVM IR, object emission, and native `.xse`
  linking.
- Official `.xhir`, `.xmir`, and `.xlil` intermediate outputs are not emitted until structural AST is complete and the
  formats are documented.
- `compilerOptions.xsBackend` optionally accepts `"LLVM"` or `"XS"`.
- `xsBackend` is currently only validated and stored in the project model; it does not affect the compilation flow or backend
  selection yet.

### Lexer and structural AST

- Documented keywords, operators, line comments, and multiline text literals are tokenized. X# accepts `//`, `///`, and
  `//!` line comments; multiline and nested comments are not supported.
- ASCII identifier rules are applied.
- Decimal integers, floating-point numbers, scientific notation, and `'` digit separators are validated.
- String and character literal source spellings are carried into AST literal nodes; resolving `Char` as a 16-bit character is
  left to the HIR type stage.
- The parser produces an arena-based structural AST.
- AST nodes carry full source location: file id, offset, line, and column.
- Declaration, type, statement, expression, pattern, and macro node families are represented.
- Outer `#[...]` attributes are parsed before declarations and declaration members into `XS_SYNTAX_ATTRIBUTE` nodes.
  File-level inner `#![...]` attributes are parsed before declarations. Attribute syntax is built in, but official X#
  attribute names live under `std::attrs::*`; semantic validation of each attribute remains a HIR/name-resolution step.
- In top-level and class-member contexts, `name!();` macro calls are represented as `XS_SYNTAX_DECL_MACRO_CALL` declaration
  nodes. This node is the entry point for item/declaration-producing macro expansion; inserting produced items as real AST
  replacements in scope is a later macro-expansion step.
- Named, generic, array, fixed array, pointer, reference, tuple, unit, and `fn(...) -> T` function type nodes are parsed into
  the structural AST.
- The lexer keeps `>>` as a shift-right operator token; the structural parser may consume that token as two separate `>`
  tokens when closing generic type/generic parameter contexts.
- Lifetime spellings in reference types follow X# forms based on Rust lifetimes (`&'a T`, `&'a mut T`, `&'static T`,
  `&'else T`) and are carried into the AST as `XS_SYNTAX_LIFETIME` nodes. X# uses `else` where Rust examples often use
  `_`; lifetime elision and validation are left to the borrow-checker stage.
- Function parameters, return type, deprecated `throws` types, and function bodies are structural nodes; bodies are not
  stored as raw ranges. The `->` return type is marked with `XS_SYNTAX_FLAG_RETURN_TYPE`; `throws` types are not
  interpreted as return types.
- `op Name(...) -> Type` is parsed as a distinct referentially transparent operation declaration. The current C23 AST stores
  it on the function-shaped declaration node with `XS_SYNTAX_FLAG_REFERENTIAL_TRANSPARENT`, but the semantic category is
  separate from ordinary `fn`.
- Top-level `extern "ABI" { fn ...; static ...; }` blocks are parsed as external declaration blocks. The structural AST
  stores the ABI string token on the block and on each contained function/static symbol. Functions are marked
  external/incomplete, and static foreign globals are marked external/static. HIR currently accepts the first explicit C ABI
  shape, `#[repr(C)] extern "C"`, and rejects unsupported ABI strings or missing/non-C representation attributes. Library
  resolution, symbol binding, and backend lowering remain HIR/backend work.
- Legacy exception syntax (`throws`, `throw`, `try`, `catch`, and `finally`) remains parseable but is deprecated and
  scheduled for removal in X# 2.0.0. The parser emits warnings for `throws`, `throw`, and `try`; new code should use
  `Result<T, E>` and postfix `@` propagation.
- `data` declaration bodies accept fields, constructors, methods, and `fn operator <token>(...)` declarations. Data
  constructors and methods form overload sets by parameter type list; identical parameter type lists produce a parser
  diagnostic. Data destructors, inheritance, and interface members remain invalid.
- Class constructor names must match the class name. Constructors may be overloaded by parameter type list; duplicate
  parameter type lists produce parser diagnostics.
- Interface declaration bodies accept only body-less function declaration signatures.
- Classes and interfaces use a C#-style `:` base list. The parser stores the base/interface entries as type children; HIR
  resolves which entry is the class base and which entries are interfaces. Legacy `extends` and `implements` spellings
  produce parser diagnostics.
- Class fields may carry `getter` and `setter` property accessors. Accessors are represented as
  `XS_SYNTAX_PROPERTY_ACCESSOR` children; accessor bodies are parsed as ordinary blocks when present. Properties use the
  canonical X# field spelling, for example `public Name: Str { getter; setter; }`.
- Visibility modifiers use C# meanings: `public` is externally visible, `private` is limited to the declaring scope,
  `protected` is available to derived classes, and `internal` is limited to the current project/package boundary. Current HIR
  symbol collection applies C# defaults: top-level declarations are `internal` unless made public by a `public namespace`
  rule or an explicit modifier, while type members are `private` unless explicitly marked otherwise. Current HIR visibility
  checks fully enforce public/private module access; protected/internal member enforcement remains part of later
  member-resolution work.
- The C23 HIR expression checker performs the first property validation slice: duplicate accessors are rejected, getter
  return values are checked against the field type where that type is currently understood, setter bodies get an implicit
  immutable `value` binding of the field type, and `self.<property>` inside that property's own accessor is rejected as
  recursive. Actual property read/write lowering remains future MIR work.
- Body-less function declarations outside interfaces require `incomplete fn ...;`. An `incomplete fn` with a body produces a
  parser diagnostic.
- Regular enum variants cannot contain payload types and must have unique names. `enum data` requires at least one typed
  variant, rejects tuple payloads, and permits same-name typed variants only when their payload types differ. Constructor
  overload selection remains a later HIR type-checking responsibility.
- `fn(...) { ... }`, `fn(...) -> T { ... }`, and `move fn(...) { ... }` function expression/closure forms are represented as
  `XS_SYNTAX_EXPR_FUNCTION` nodes. `move` capture is a separate AST flag.
- `new()` object creation is represented as `XS_SYNTAX_EXPR_NEW`; when the constructed type is not written in source, HIR will
  resolve it from context.
- Data object initialization uses normal object field literals, and field access uses ordinary member access.
- Postfix Result propagation syntax, `expression@`, is represented as `XS_SYNTAX_EXPR_RESULT_PROPAGATION`. The C23 HIR
  expression checker now requires an enclosing function whose return type is `Result<T, E>` or shorthand
  `Result<T, E>`. It does not yet prove that the propagated operand itself has a matching Result type; full propagation
  control-flow lowering is handled by later HIR/MIR work.
- `if`, `for`, for-each, `while`, `match`, deprecated `try`/`catch`/`finally`, `return`, deprecated `throw`, `break`,
  `continue`, and `else: expression;` are parsed. The `else:` statement explicitly discards its expression value, analogous
  to a Rust discard binding, but X# spells the discard/default position as `else`.
- Expression statements follow Rust's semicolon split: `expression;` is marked with `XS_SYNTAX_FLAG_DISCARDED`, while a
  final block expression without `;` stays value-producing and is the input for implicit-return desugaring.

### Module discovery and import graph

- The project root is the directory containing the active `.xsproj` file.
- `.xs` files under the project root are scanned recursively.
- Modules are registered by their declared full module path, not by file name.
- Declaring the same module name in multiple files is an error.
- `imports` and `using` dependencies are resolved by declared module name.
- Missing import targets produce errors.
- Imported sources that are not listed in `addFiles` are added to the dependency graph and checked.

This layer lives under the HIR directory.

### HIR symbol collection

- The `xs check` flow runs HIR symbol collection after structural AST and macro validation.
- During project checking, direct sources and sources discovered through the import graph are collected into one shared HIR
  symbol table.
- File-level `module` and `namespace` declarations determine the active HIR namespace path.
- If a file uses `public namespace`, top-level declarations without an explicit visibility modifier are treated as public in
  HIR.
- `public namespace` does not override explicit `private`, `internal`, or `protected` visibility modifiers.
- Top-level `fn`, `class`, `interface`, `enum`, `data`, and `macro_rules!` declarations are collected into the symbol table.
- `macro_rules!` rules use `matcher -> { expansion };`; the older `matcher: { expansion };` spelling is not accepted.
- `extern "ABI"` block functions are collected as function symbols, while `static` foreign globals are collected as
  `extern global` symbols. Visibility on the extern block is inherited by contained declarations unless a contained
  declaration has its own explicit visibility.
- HIR CFFI validation runs after import resolution. Version 0 currently requires C ABI extern blocks to be written as
  `#[repr(C)] extern "C" { ... }`; this keeps the ABI/layout intent explicit before backend CFFI lowering exists.
- The same pass validates the first standard CFFI attribute shapes and scopes: block metadata such as `LinkLibrary` and
  `Header`, function metadata such as `LinkName` and unwind/thread markers, and static metadata such as `ThreadLocal`.
- `xs_hir_collect_symbols_expanded`, when given a declaration macro expansion set, collects synthetic declarations produced
  by `XS_SYNTAX_DECL_MACRO_CALL` reparse trees into the active HIR namespace of the macro call. Duplicate symbol checks use
  the same namespace rule as normal declarations.
- `xs_hir_collect_member_symbols` produces a separate HIR member symbol table for a class/interface owner symbol. V0 scope
  covers fields, field-like macro output, methods, constructors, destructors, and nested types.
- If methods repeat with the same name, the last declaration wins in lookup according to the X# method-merge rule; field and
  nested-member name conflicts produce diagnostics.
- Symbols carry short name, namespace name, fully qualified name, visibility, source location, and source AST node.
- Duplicate top-level declarations with the same short name in the same namespace are errors.
- The same short name may be used under different namespaces.
- `imports Module;` records that the module is usable through its qualified name. It does not place public symbols into the
  local import scope.
- Source syntax uses Rust-style qualified paths: `Module::Name` and `Module::Type::Item` are namespace/type paths, while
  `value.member` remains value member access. Expression generic arguments use turbofish, for example
  `Factory::<Int>()`. HIR still stores canonical qualified names with `.` internally.
- `using Module::Name;` and `using Alias = Module::Name;` open one public top-level symbol into the local import scope.
- `using namespace Module;` opens public top-level symbols from that namespace into the local import scope.
- Non-public symbols are not opened through external module imports.
- Qualified external names and types require a matching `imports Module;` or `imports Module::Namespace;` declaration unless
  they share the same root module as the current namespace.
- Direct call targets in function bodies are resolved through same-namespace symbols and the import scope.
- Fully qualified call targets through another namespace/module resolve only to public symbols.
- Non-public symbols can be resolved only from the same namespace and same source file through direct qualified names.
- Calls where the first segment is a local parameter/variable are deferred to type checking.
- `value.Method()` calls on locals/parameters with explicit named type annotations are validated for member existence through
  the HIR member symbol table. At this stage the receiver type is resolved only for direct identifier receivers and named type
  annotations.
- After declaration macro expansion, the HIR member symbol table includes field-like variables and method declarations
  produced inside classes/interfaces in normal member declaration order. Macro-generated member names that conflict with
  original members produce diagnostics; only method-method same-name cases remain merge/overload candidates.
- `xs_hir_validate_name_uses_expanded`, when given a statement macro replacement set, traverses direct statement child lists
  through the `xs_macro_expand_child_statements` expanded view. This validates function/method call targets produced after
  macro expansion against the HIR symbol/import scope.

This stage does not yet perform method/operator resolution, overload selection, generic constraint membership resolution, or
type-based call resolution. The member symbol table is the first HIR data model for those later stages; method existence
validation does not decide dispatch, override, or overload selection.

### HIR type resolution bootstrap

- The `xs check` flow runs HIR type resolution after HIR import and name resolution.
- The current primitive type names are recognized: `Str`, `Bool`, `Byte`, `SByte`, `Char`, `Short`, `Long`, `Int`,
  `Integer`, `UShort`, `ULong`, `UInt`, `UInteger`, `SFloat`, and `Float`.
- `Bool` is resolved as a 1-bit primitive in HIR; the LLVM backend lowers it to `i1`.
- `Byte` is an unsigned 8-bit primitive and `SByte` is a signed 8-bit primitive at the HIR level.
- `Char` is a 16-bit UTF-16 code-unit type.
- Signed integer widths are `Short`/`Long`/`Int`/`Integer` = i16/i32/i64/i128; unsigned widths are
  `UShort`/`ULong`/`UInt`/`UInteger` = u16/u32/u64/u128. `SFloat` and `Float` map to f32 and f64.
- `Str` is encoded as UTF-16; the compiler/runtime selects UTF-16LE or UTF-16BE automatically for the target/runtime
  situation. Its length is considered unbounded except by the representation allowed by UTF-16.
- Semantically, `Str` is the X# counterpart of Rust's `&'static str`: an immutable static string reference. Its runtime
  layout remains deferred and it is not yet lowered to XLIL storage.
- `Optional<T>` resolves as if the compiler had inserted `imports optional; using namespace std::optional;`, making
  `std::optional::Optional<T>` available as `Optional<T>`. Optional value constructors are canonically
  `std::optional::None` and `std::optional::Some(...)`, with `None` and `Some(...)` available through that implicit
  namespace using. It is not an enum
  lowering. `?.`, `??`, `??=`, and postfix `!` are represented syntactically; `Optional<T>` has automatic unboxing to
  `T`, and failed unboxing is modeled through the standard `Result`/`Error` direction rather than the deprecated exception
  system. `Optional<Str>` is special at the language model level: it behaves like Rust's `Option<String>`, meaning the
  optional string payload is owned/dynamic instead of an optional borrowed/static `Str` reference. Full flow-sensitive
  Optional semantics are later HIR work.
  There is no nullable `T?` type operator.
- The C23 HIR type resolver recognizes the standard wrapper type names `Optional<T>`, `std::optional::Optional<T>`,
  `std::result::Result<T>`, `std::result::Result<T, E>`, shorthand `Result<T>`, and the standard error type `Error`.
  The compiler treats the `std::result` namespace as implicitly usable, so `imports result;` is optional for `Result<T>`,
  `Result<T, E>`, `Ok(...)`, and `Error(...)`. Unit success is canonically written as `Result<()>`. This is name and
  arity validation only; constructors, method calls, and propagation lowering are still handled by later semantic passes.
- `Panic` is treated as an implicit standard import for the assertion and panic macro family. Those macros remain normal
  imported macros rather than built-ins; the macro validator simply treats the `Panic` module as always available.
- `Stdio` is intentionally not prelude. `print!`, `println!`, `eprint!`, `eprintln!`, `write!`, `writeln!`, and `format!`
  still require `imports stdio;` or `using namespace stdio;`. `format_args!` remains built in.
- The C23 HIR type resolver also recognizes the initial standard CFFI family: `std::cffi::CStr`, `std::cffi::CString`,
  `std::cffi::RawPtr<T>`, `std::cffi::NonNull<T>`, `std::cffi::Slice<T>`, `std::cffi::Handle<T>`, `std::cffi::Owned<T>`, `std::cffi::Borrowed<T>`,
  `std::cffi::Out<T>`, `std::cffi::DynamicLibrary`, `std::cffi::Symbol<T>`, `std::cffi::File`, and `std::cffi::VarArgs`.
- X# uses nominal typing. HIR type identity for user-defined types is based on name/symbol identity; identical structural
  shape does not imply compatibility.
- HIR primitive metadata carries XLIL type mappings for primitive types with documented runtime layout.
- `Str` is not mapped to an XLIL type yet because its runtime/ABI layout is incomplete.
- Type names inside functions, data, enum, class/interface members, and generic constraints are validated.
- Generic parameter names are recognized in their own declaration scope.
- User-defined `class`, `interface`, `enum`, and `data` types are resolved through the HIR symbol table and import scope.
- Fully qualified type uses through another namespace/module can resolve only to public type symbols.
- Non-public type symbols can be resolved only from the same namespace and same source file.
- Generic type uses must provide the same number of type arguments as the declaration’s generic parameter count.
- Because generic type erasure and default generic parameters are not supported, using generic types without arguments is an
  error.
- Duplicate generic parameter names in the same declaration scope are errors.
- Generic constraint types must resolve to interface symbols. For generic interface constraint uses, arity and type argument
  resolution are also performed during HIR type resolution.
- Generic parameters may carry multiple constraints. In a constraint list, `, Identifier :` starts a new generic parameter;
  otherwise the comma separates another constraint on the same parameter.
- `xs_hir_check_expression_types_with_macros` checks literal compatibility for variable initializers with explicit primitive
  type annotations and direct assignment literal RHS values to the same local. For now, integer, float, string, char, and
  bool literal kinds are matched against primitive target types. `return <literal>;` statements inside functions with
  explicit primitive return types are checked with the same literal compatibility logic. `None`, `new`, identifiers, calls,
  and other expression forms are deferred until general expression type inference is complete.
- The same HIR expression-check stage reports diagnostics for direct identifier assignment to `val`, `const`, and `static`
  immutable declarations inside local block/function scopes. Field, index, dereference, alias/borrow-based mutability rules
  are deferred to later borrow/type-check stages.
- `op` bodies are checked for the first referential-transparency slice. Calls, method calls, assignment, macro calls, `new`,
  `await`, result propagation, mutable borrow, property accessors, and legacy exception syntax
  are rejected inside `op` until a fuller effect/purity model can classify them precisely.
- `xs_hir_resolve_types_expanded`, when given a statement macro replacement set, traverses direct statement child lists
  through the `xs_macro_expand_child_statements` expanded view. This validates type uses produced after macro expansion
  against the HIR symbol/import scope.

This stage does not yet produce general expression type inference, overload selection, constraint membership/compatibility
checks, trait/interface compatibility, or ABI/layout decisions.

The growing semantic-analysis and type-checking implementation now starts in the isolated Rust `xslang` crate instead of
adding new semantic rules to the old C23 HIR prototypes. The first checked Rust rule validates that `await` expressions occur
only inside async function bodies. `xslang` also carries the first Result propagation type rule: `Result<T>@` and
`Result<T, E>@` have success type `T`, require an enclosing Result return type with a compatible error channel, and remain
deferred at HIR-to-MIR lowering until error-return control-flow lowering exists. The crate is not wired into the C23 driver
yet; integration will use a bulk structural syntax transfer boundary so one compiler layer is not split across C and Rust.

The C23 HIR prototype mirrors the first parts of that rule: `@` is accepted inside functions returning
`Result<T>`/`Result<T, E>` and rejected elsewhere. When the operand is a direct same-file function call, the
callee must also return a Result type. Imported calls, method calls, local function values, and full success/error
compatibility remain the Rust compiler-core direction and later C/Rust integration work.

`@` is a surface-language sugar. In `xslang`, the Result desugar pass translates it into an explicit Result-match and
early-return intent model before MIR lowering. If a raw `ResultPropagation` expression reaches MIR lowering, that is treated
as a pipeline ordering error and is rejected instead of becoming a backend primitive.

For `xslang` propagation checking and desugaring, single-argument `Result<T>` uses the standard `Error` error type.

The MIR lowerer has a separate `DesugaredFunction` entry point. It can lower desugared functions that contain only currently
supported expression forms, but explicit ResultMatch nodes remain deferred until Result-aware MIR control-flow and return
construction are implemented.

XHIR text writing can emit desugared functions for inspection. The desugared form writes `result_match` records with explicit
success/error binding names and types, making `@` expansion visible without pretending that MIR Result control flow is
complete.

### Macro validation and scope resolution

- Macro matcher variables, repetition depths, and expansion variables are validated.
- Direct and indirect macro recursion in the same scope is an error.
- Macro definitions are collected for the relevant scope before expansion.
- Macro fragment/token matcher helpers are separated into `xs/sources/macro/fragment.c`; validation, expansion traversal, and
  future fragment reparse support must grow without bloating `xs/sources/macro/expansion.c`.
- Macro calls before textual definition in the same scope are accepted.
- Macros defined in an inner scope cannot be called from an outer scope.
- Calls are checked to resolve to a visible macro definition.
- `include!` and `format_args!` are built-in macros. `format_args!` uses the same Rust 1.57-style format string validation
  as Stdio formatting macros.
- Imported `Stdio` macros are treated as external macros, not built-ins. The validator recognizes `print!`, `println!`,
  `eprint!`, `eprintln!`, `write!`, `writeln!`, and `format!` through `imports stdio` or `using namespace stdio;`. `println!()`
  and `eprintln!()` accept the Rust 1.57 newline-only form, while `writeln!(destination)` accepts the destination-only
  newline form. The other Stdio formatting forms require a string literal format template and matching placeholder argument
  count. Rust 1.57-style debug/pretty-debug and common formatting specs such as `{:?}`, `{:#?}`, `{:08x}`, and `{:_>8}`
  are accepted by the validator.
- Full token matcher rules and empty matcher rules can be matched.
- Single-token fragment matchers that can be validated precisely (`tt`, `ident`, `literal`, `lifetime`, and `vis`) are matched
  against call tokens.
- The macro expansion preparation API is `xs_macro_prepare_expansion`. It resolves calls in scope, counts calls that can be
  expanded without structural reparsing through single-token fragments or full-token matchers, produces a simple expansion
  token/substitution plan, and intentionally defers `meta` fragments.
- V0 support for `expr`, `stmt`, `block`, `ty`, `path`, `item`, and `pat` fragments captures the call-parentheses token
  sequence as a single expression/statement/block/type/path/item/pattern fragment when the matcher contains exactly one
  remaining fragment. `$name` use in expansion carries that token sequence to statement or declaration reparse.
- `xs_macro_expand_tokens` produces call span and expanded token lists for simple supported calls. A macro call must match
  exactly one rule; zero matches and multiple matches are validation errors. This output is not written back into the
  structural AST yet; it is an intermediate flow for later fragment-level reparse and AST replacement.
- `xs_macro_reparse_expansion_as_statement` reparses a supported expansion token list as a statement inside a synthetic
  function body. This is a bridge for fragment-level reparse and AST replacement, not final macro-call replacement.
- `xs_macro_reparse_result_statement` extracts the replacement statement node from the reparsed synthetic body. Parent-child
  AST replacement remains a later step.
- `xs_macro_expand_statements` reparses supported token expansions as statements and maps call span to replacement statement
  nodes inside `XsMacroStatementExpansionSet`. The set owns the synthetic reparse trees, so replacement nodes remain valid
  until the set is freed. This API only produces statement replacements for statement-context macro calls; nested macro calls
  inside other expressions remain token-level expansions.
- `xs_macro_statement_expansion_find` returns the synthetic replacement statement node for an `XS_SYNTAX_STMT_MACRO_CALL`.
  HIR consumers do not keep their own span mapping code for replacement lookup.
- `xs_macro_expand_declarations` reparses supported token expansions from declaration-context `XS_SYNTAX_DECL_MACRO_CALL`
  nodes as synthetic source files and stores call span, reparse tree ownership, and produced declaration count inside
  `XsMacroDeclarationExpansionSet`. `xs_macro_declaration_expansion_find` returns the synthetic declaration expansion record
  for a declaration macro call node.
- `xs_macro_expand_top_level_declarations` produces a structural expanded view for a top-level declaration list. The view
  preserves original declaration nodes, materializes `XS_SYNTAX_DECL_MACRO_CALL` nodes through synthetic declaration
  expansion records with matching call spans, and provides a testable expanded-AST bridge until parent-child AST rewrite is
  complete.
- `xs_macro_expand_child_declarations` provides the same expanded-view mechanism for the direct children of any parent
  declaration. HIR type resolution traverses function members and field-like variable declarations produced by declaration
  macro calls inside `class` and `interface` through this view.
- `xs_macro_expand_child_statements` produces a structural expanded view for direct statement children of nodes such as
  statement blocks. The view preserves original statement nodes and materializes `XS_SYNTAX_STMT_MACRO_CALL` nodes through
  synthetic replacement statement records with matching call spans in statement order. This API does not physically rewrite
  parent-child AST; it is an intermediate bridge shared by HIR/MIR passes that consume statement macro replacement.
- `xs_macro_materialize_expanded_tree` clones an expanded tree into a separate syntax tree, replacing macro-call
  declaration/statement nodes with replacements from declaration and statement expansion sets. It does not mutate the
  original AST; replacement text/span references in the materialized tree are tied to the expansion set lifetime because the
  expansion sets own the reparse source text.
- HIR symbol collection operates through the top-level expanded declaration view. Declaration macro calls must have one
  matching rule before their replacement declarations enter the normal declaration flow.
- The `xs check` flow runs macro validation, macro expansion preparation, and statement expansion set generation before HIR
  symbol collection. The driver keeps the replacement set alive for the compilation unit and passes it to HIR name-use and
  HIR type-resolution traversals. HIR name and type resolution traverse statement child lists through the expanded statement
  view, so each validated statement macro replacement enters the normal statement flow.

Declaration/item-context macro-call AST input, declaration reparse set generation, top-level/child declaration expanded views,
statement expanded view, separate expanded tree materialization, HIR symbol-collection integration, and class/interface
function/field-like member expansion wired into HIR type traversal all exist. Writing generated declaration/statement nodes
back as in-place parent-child replacements on the main AST and reclassifying field-like declarations as `XS_SYNTAX_CLASS_FIELD`
remain next steps.

`meta` fragment capture and full AST expansion are still incomplete. `expr`, `stmt`, `block`, `ty`, `path`, `item`, and
`pat` fragment support is currently limited to a single token sequence. Unsupported fragment matchers do not invent
semantics.

### MIR model

- `xs/mir.h` exposes a C23 API for modules, function declarations, function definitions, basic blocks, and terminators.
- MIR function definitions carry a basic block list.
- MIR function definitions carry a local table; local kind, type, mutability, and name are stored.
- The MIR place model starts with a root local plus a `field`/`deref`/`index` projection chain.
- MIR has an SSA value table and core `const.i64`, `const.bool`, signed i64 arithmetic/bitwise/shift/comparison
  instructions, `load`, and `store` instructions.
- Each basic block may currently have a `return`, `goto`, `branch_if`, `panic`, or `unreachable` terminator in the Rust MIR
  model. The C MIR model also distinguishes `panic` from structurally unreachable control flow.
- The MIR text writer deterministically writes declarations and functions with bodies.
- `xs/mir/borrow_checker.h` contains the first MIR validation/borrow-check skeleton.
- The borrow-checker skeleton validates mandatory terminators, return type compatibility, and `store` operations into
  immutable local roots.
- The borrow checker also validates instruction result/value ids, `load`/`store` place ids, `goto`/`branch_if` targets,
  `branch_if` condition liveness/type, i64 arithmetic/bitwise/shift operand type/id consistency, and i64 comparison
  i64-to-bool result consistency.
- `xs/mir/optimizer.h` contains the initial MIR optimization API.
- The CFG cleanup pass removes blocks unreachable from the entry block and rewrites remaining block ids plus `goto`/`branch`
  targets.
- Constant folding lowers safe i32/i64 arithmetic, bitwise, and shift instructions with two matching constants to a
  constant result, and lowers i32/i64 comparisons with two matching constants to a `const.bool` result.
- Constant folding also lowers a MIR `branch_if` whose condition resolves to a known `const.bool` into a direct `goto`;
  CFG cleanup can then remove the dead target block.
- Source-native builds run C MIR borrow checking, constant folding, CFG cleanup, and a second borrow-check pass before
  lowering MIR to XLIL.
- Rust `xslang` also contains a target-independent MIR structural verifier for duplicate local/block ids, missing
  terminators, unknown local references, and unknown block targets. This verifier is separate from LLVM and runs before
  borrow-check-specific reasoning.
- Rust `xslang` optimizer exposes `optimize_verified_function`, which validates MIR before optimization and verifies the
  optimized MIR before returning it.
- Rust `xslang` XMIR text support can write and parse optimizer analysis records for optimization pass reports.
- Rust `xslang` XMIR text support can also write and parse structural verifier diagnostic analysis records.
- Rust `xslang` MIR and XLIL models now carry `add.i64`, `sub.i64`, `mul.i64`, `const.bool`, and `eq.i64`. XMIR and XLIL
  text parsers/writers round-trip them, the MIR verifier checks operand/result types, MIR → XLIL lowering emits matching
  XLIL instructions, and the Rust MIR optimizer folds arithmetic and equality comparisons when operands are known
  `const.i64` values.
- Rust `xslang` MIR and XLIL models also carry the first conditional control-flow primitive. MIR writes/parses
  `branch_if` records with `condition local N`, `then block A`, and `else block B`; XLIL writes/parses assembly-like
  `br_if %rN, bbA, bbB` terminators. Verifiers require a `bool` condition and existing target blocks. HIR lowering does not
  emit conditional control-flow yet. The MIR optimizer can fold same-block constant `branch_if` conditions to `goto`.
- Rust `xslang` contains the first target-independent HIR to MIR bridge. It lowers void functions, `Long`/`Int` locals,
  non-negative `Long` i32 and `Int` i64 literals, and local returns into a single-entry MIR block with typed
  XLIL-vocabulary local records. Negative literal/unary-expression lowering and other integer widths remain deferred.
  Unsupported HIR expressions and primitive values whose runtime layout is not ready, such as `Str`, produce lowering
  diagnostics instead of inventing temporary backend semantics.

This stage does not yet produce complete statement/expression lowering, the full instruction set, exception edges, async
state machine generation, region/loan/move analysis, drop-point validation, or a comprehensive MIR optimization pass set.

### LLVM backend infrastructure

- A separate `xs_backend_llvm` library exists outside the frontend.
- LLVM context, target machine, target triple, and data layout are managed.
- A separate LLVM module is created for each codegen unit.
- Documented numeric primitive types map to LLVM types.
- Body-less function declarations and function signature lowering are supported.
- XLIL function declaration signatures lower through the XLIL type vocabulary rather than through HIR primitive types.
- LLVM optimization pipelines from `default<O0>` through `default<O3>` can be configured.
- LLVM module verification, LLVM IR text emission, and object file emission work.
- `xs build --xlil -file <input.xlil>` parses and verifies XLIL v0 text through `xs_lil_module_parse_text`, then lowers to
  LLVM IR, verifies and optimizes the LLVM module, and emits an object file and native `.xse` executable beside the input.
  Native direct XLIL requires exactly one defined
  `.func main : () -> i32`; its supported body subset includes `.param`, `const i64`, `const.i32`, `const.bool`,
  `add.i32`, `sub.i32`, `mul.i32`, `div.i32`, `rem.i32`, `and.i32`, `or.i32`, `shl.i32`, `shr.i32`, `eq.i32`,
  `ne.i32`, `lt.i32`, `le.i32`, `gt.i32`, `ge.i32`, `not.bool`, signed i64 arithmetic/bitwise/shift/comparison
  instructions, typed `.slot`/`load`/`store`, `call`, `br`, `br_if`, `panic`, `ret`, and `ret %rN`.
- `xs build -file <input.xs>` and `xs build -proj <input.xsproj>` can now use the same native path for the first checked
  source slice: one top-level `main` returning `Long` with optional explicit `Long`/`Bool` or inferred local bindings
  followed by one return statement whose expression is built from locals, i32-range integer literals, unary `+`/`-`,
  arithmetic, bitwise including `^`, shift, and top-level `if` expressions. This source bridge creates a temporary C MIR
  function, lowers it to XLIL, and then reuses the XLIL native builder.
- Direct executable linking uses the configured Clang driver with LLD for the native Linux ELF target. A configured
  cross-target still receives LLVM IR and object artifacts, then stops before executable linking; runtime and external
  library linking remain unconfigured.
- `.xse` is the native executable artifact extension. The first output format under that extension is ELF; PE support comes
  later.
- The linker can be invoked without a shell, with argument policy left to the upper layer.

Details: [LLVM_BACKEND.md](LLVM_BACKEND.md)

### XLIL target

- `docs/XLIL.md` defines XLIL as the official low-level intermediate language for X#.
- `.xhir`, `.xmir`, and `.xlil` are the extensions for HIR, MIR, and XLIL code. They are human-readable text formats, not
  binary or opaque serialized compiler state.
- Their leading `version N` records are compiler input contracts for `xs build`; they select the supported XHIR/XMIR/XLIL
  text grammar version to parse. Version `0` is the only supported version today, but the parser structure is open to adding
  later versions explicitly.
- `.xhir` and `.xmir` are intended for direct developer inspection, code review, regression fixtures, and debugging. Their
  official record grammar is not fully documented yet, but future grammar changes must keep that human-readable property.
- `.xhir` and `.xmir` are not assembly-like. XHIR should expose resolved semantic structure; XMIR should expose structured
  control-flow, places, drops, regions, and analysis results. See [XHIR.md](XHIR.md) and [XMIR.md](XMIR.md).
- XLIL is the target-independent type/data vocabulary that HIR/MIR depend on.
- XLIL is designed to sit before LLVM IR and act as the common input point for backends.
- `.xlil` is always a text registry format; no binary XLIL format will be added.
- XLIL is not as high-level as CLR and not as low-level as assembly; it is an assembly-like but distinct
  target-independent mid/low-level registry language.
- XLIL is not bytecode or a virtual-machine format.
- The stable XLIL registry/generation C23 API target is documented as `#include <xs/lil.h>`.
- The XLIL AOT C23 API target is separated as `#include <xs/lil/aot.h>`; until concrete object/link behavior exists, it only
  marks the planned public surface.
- External frontends and tools should eventually be able to produce XLIL and use the XS LLVM backend, and later the XS
  Backend, to produce native executables.
- Third-party languages can generate XLIL through `xs/lil.h`; XLIL AOT, HIR baseline JIT, and MIR performance JIT are planned
  as separate public headers: `xs/lil/aot.h`, `xs/hir/jit.h`, and `xs/mir/jit.h`.
- Direct file compilation entries are recognized as `xs build --output hir|mir|xlil -file <input>` and
  `xs build --hir|--mir|--xlil -file <input>`. The commands are not fully implemented yet.
- `xs/lil.h` contains target-independent core APIs for XLIL modules, verification, primitive types, text parsing/writing,
  read-only function/body inspection, function bodies, basic blocks, parameters, `const.i64`, `const.i32`, `const.bool`,
  typed stack slots and `load`/`store`, i32 arithmetic/bitwise/shift/comparison, i64 arithmetic/bitwise/shift/comparison,
  `call`, `br`, `br_if`, and `return`.
- The XLIL text writer emits assembly-like registry records: `.xlil version 0`, `.xlil module`, `.extern`, `.func`,
  `.slot %sN:type`, `bbN.label:`, typed SSA instructions, `store %rN, %sA`, `%rN:type = load %sA`, `br bbN`,
  `br_if %rN, bbA, bbB`, `panic`, `ret`, and `.end`.
- MIR parameters carry an explicit immutable local id plus XLIL-vocabulary type, allowing the first MIR → XLIL body bridge
  to bind them to XLIL `.param` values. The bridge lowers MIR
  parameter signatures, typed MIR `const.i64`, `const.i32`, `const.bool`, i64 arithmetic/bitwise/shift/comparison local
  statements,
  typed call statements whose arguments already have lowered XLIL values, unconditional `goto`, conditional `branch_if`
  with an already-lowered `bool` condition, and matching local return values to XLIL signatures, `const`, arithmetic,
  compare, `call`, `br`, `br_if`, `panic`, and `ret %rN` records. C MIR `panic` lowers to the XLIL terminator; LLVM then
  emits `llvm.trap` and `unreachable`.
- C MIR root-local places lower to typed XLIL stack slots. Root-place loads and stores lower through XLIL and LLVM
  `alloca`/`load`/`store`; field, dereference, and index projections remain deferred until their layout rules are available.
- `xs/mono/plan.h` contains an initial monomorphization plan API. For now it only binds already concrete MIR functions to
  stable `_XS_FN_..._G0` symbol names; reachable generic instantiation generation is next.
- `xs/codegen/units.h` contains a target-independent codegen-unit planning API. MIR functions are split into module-path
  based codegen units by the default v0 policy. When produced from a mono plan, the unit name comes from the source module
  path and the function name comes from the stable monomorphized symbol.
- Rust `xslang` contains the first XHIR/XMIR structured text writer and header parser bootstrap. These outputs are
  intentionally separate from XLIL's assembly-like registry records.
- Rust `xslang` XHIR text support can write and parse type-check diagnostic analysis records.

XLIL instruction set, function body model, runtime/ABI layout, and MIR → XLIL body lowering are implemented incrementally.
Public XLIL behavior is documented in [XLIL.md](XLIL.md).

## Unfinished stages

The public roadmap is summarized in [TODO.md](TODO.md). Remaining work is implemented incrementally while keeping documented
syntax, public APIs, and the current architecture stable.

- Macro fragment matcher engine and AST macro expansion
- HIR method and operator resolution
- Type, function-call, generic, and trait/interface dependency edges beyond module/import
- Expression type checking and generic constraint validation
- Send, Sync, mutability, and async/await validation
- Complete MIR statement/expression lowering, exception paths, and async state machine generation
- Borrow checker and drop-point validation
- MIR optimizations
- Monomorphization, codegen unit splitting, and incremental compilation cache
- XLIL function body data model and MIR → XLIL body lowering
- XLIL to LLVM IR function body lowering
- LLVM mapping for `Str`, whose runtime/ABI layout is not fully implemented
- Linking generated object files according to project targets
- End-to-end source-project `xs build` and `xs run`

Temporary language rules are not invented in the parser or LLVM backend for unfinished semantic stages.

## Build and test

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug
```

To check the example project:

```text
./build/clang-debug/xs check -proj tests/fixtures/example_project/MyApp.xsproj
```
