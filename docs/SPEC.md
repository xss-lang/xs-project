<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# X# specification examples

The `Spec/` tree is the public language-example area for X#. It is intentionally example-driven: files show surface syntax,
semantic intent, valid and invalid shapes, and larger complete-language sketches.

The compiler is not expected to compile every example today. Examples may describe the completed language, but documented
syntax takes priority over ad-hoc implementation shortcuts.

## How to read `Spec/`

- `Spec/TypeSystem.xs` covers primitive names, nominal typing, `Optional<T>`, and standard wrapper behavior.
- `Spec/Variables.xs`, `Spec/Functions.xs`, and `Spec/ControlFlow.xs` cover everyday local/function/control-flow syntax.
- `Spec/Macros.xs`, `Spec/Attrs.xs`, `Spec/Stdio.xs`, and `Spec/Panic.xs` cover macro behavior, attributes, and imported
  macro modules.
- `Spec/Data.xs`, `Spec/Enum.xs`, `Spec/Classes.xs`, and `Spec/Generics.xs` cover declaration forms.
- `Spec/FS.xs`, `Spec/Http.xs`, `Spec/Process.xs`, `Spec/Sync.xs`, `Spec/Thread.xs`, and `Spec/CFFI.xs` describe
  standard-library-facing examples.
- `Spec/Programs/` contains larger one-file programs written as if the completed language and standard library existed.

## Current example conventions

- Macro names are snake_case and are not under `STD`.
- Standard modules use `std::<module>` names, such as `std::fs` and `std::collections`.
- `::` separates modules, namespaces, types, associated items, and
  constructors; `.` is reserved for value member access and method calls. Generic arguments in expression paths use
  turbofish, for example `std::optional::Some::<Str>("value")`.
- Instance members use `self` as the receiver name inside constructors, methods, and property accessors.
- Property bodies should use explicit backing fields such as `self._age`; referencing the property itself as `self.age`
  inside its own accessor is recursive and rejected.
- X# evaluation is strict/eager. The language aims for referential transparency where practical, but standard-library APIs
  are not globally guaranteed to be referentially transparent.
- `fn Name(...) -> Type` declares an ordinary function. `op Name(...) -> Type` declares a referentially transparent
  operation for pure computation; operations are not just aliases for functions, and non-transparent operation bodies are
  compile-time errors.
- `imports Module;` makes a module usable through its qualified name. It does not place the module's public symbols in
  local scope. For example, `imports fs;` keeps file APIs qualified as `std::fs::File::open(...)`; users may add
  `using namespace std::fs;` when they want `File::open(...)`. Use `using Module::Name;`, `using Alias = Module::Name;`,
  or `using namespace Module;` when short names are desired.
- `include!`, `format_args!`, `format_args_nl!`, `write!`, and `writeln!` are built-in macros. `format_args!` and
  `format_args_nl!` are compiler-special and cannot be declared or shadowed through `macro_rules!`.
- `print!`, `println!`, `eprint!`, `eprintln!`, `format!`, and the `std::fmt::*` API come from `Stdio`.
- Attribute delimiter syntax is built in: `#[...]` applies to the following declaration/member, and `#![...]` applies to the
  enclosing file/module form. Official X# attributes live under `std::attrs::*`; the compiler brings those names into
  attribute scope automatically, so `imports attrs;` is optional.
- CFFI is explicit. The standard CFFI module is not automatically imported or placed in scope; source uses `imports cffi;`
  before C foreign-function declarations and the canonical helper types under `std::cffi::*`.
- `Optional<T>` is available as if the compiler had inserted `imports optional; using namespace std::optional;`, making
  `std::optional::Optional<T>` available as `Optional<T>`.
- `Str` is the X# UTF-16 equivalent of Rust's `&'static str`; the compiler/runtime selects UTF-16LE or UTF-16BE
  automatically for the target/runtime situation. `Optional<Str>` is modeled like Rust's `Option<String>` with an owned
  optional string payload.
- X# uses `else` for placeholder/default positions. Type and lifetime placeholders are written as `Type<else, Foo>` and
  `&'else T`; `_` placeholders are not canonical X# syntax.
- Statement/expression separation follows Rust: `expression;` evaluates and discards the value, while the final expression
  in a block may omit `;` and becomes the block value. Function tail expressions are desugared as implicit returns when the
  function return type expects a value.
- Optional value constructors are `std::optional::None` and `std::optional::Some(...)`; the same implicit namespace using
  may make `None` and `Some(...)` available in source.
- `Result` is a special standard namespace: `imports result;` is optional, and the compiler behaves as if
  `using namespace std::result;` existed for `Result<T>`, `Result<T, E>`, `Ok(...)`, and `Error(...)`. Most other `std::*` modules are
  not automatically placed in local scope. `Result<T>` uses the standard `Error` channel; unit success is written as
  `Result<()>`. Result is not a default function return: a function must declare it when its body uses `@`, `Ok(...)`, or
  `Error(...)`.
- `Panic` is also an implicit standard import for assertion and panic macros. `assert!`, `assert_eq!`, `assert_ne!`,
  `debug_assert!`, `debug_assert_eq!`, and `panic!` are available without an explicit import, but they are still library
  macros rather than compiler built-ins.
- `Stdio` is not prelude and is not implicit. Its `print!`, `println!`, `eprint!`, `eprintln!`, and `format!` macros and
  `std::fmt::*` API require `imports stdio;` or `using namespace stdio;`. The writer and formatting-argument built-ins do
  not.
- Legacy exception syntax is deprecated and scheduled for removal in X# 2.0.0. Active examples should prefer
  `Result<T>`/`Result<T, E>` plus postfix `@` propagation; old `throws`/`throw`/`try`/`catch` spellings should appear only in
  explicitly marked legacy notes.

## Program examples

Each file in `Spec/Programs/` contains one example program. They are design fixtures for syntax, APIs, and semantic intent,
not compatibility promises for the current partial compiler implementation.

The current set intentionally covers different language surfaces:

- CLI/file workflows: `TodoCli.xs`, `FileBackup.xs`, `StaticSiteReport.xs`, `LogAggregator.xs`
- Data-heavy services: `BankLedger.xs`, `InventoryService.xs`, `CsvAnalytics.xs`
- Network and async sketches: `ChatServer.xs`, `HttpHealthMonitor.xs`, `SensorStream.xs`, `JobQueue.xs`
- Package/tooling flows: `PackageResolver.xs`, `PluginPipeline.xs`
