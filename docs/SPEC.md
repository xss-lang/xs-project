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
- Standard modules use `std.<Module>` names, such as `std.fs` and `std.collections`.
- `imports Module;` makes a module usable through its qualified name. It does not place the module's public symbols in
  local scope. Use `using Module.Name;`, `using Alias = Module.Name;`, or `using namespace Module;` when short names are desired.
- `format_args!` and `include!` are built-in macros.
- `print!`, `println!`, `eprint!`, `eprintln!`, `write!`, `writeln!`, and `format!` come from `Stdio`.
- Attribute delimiter syntax is built in: `#[...]` applies to the following declaration/member, and `#![...]` applies to the
  enclosing file/module form. Official X# attributes live under `std.Attrs.*`; the compiler brings those names into
  attribute scope automatically, so `imports attrs;` is optional.
- CFFI is explicit. The standard CFFI module is not automatically imported or placed in scope; source uses `imports cffi;`
  before C foreign-function declarations and the canonical helper types under `std.cffi.*`.
- `Optional<T>` is available as if the compiler had inserted `imports optional` and brought
  `std.optional.Optional<T>` into scope as `Optional<T>`.
- Optional value constructors are `std.optional.None` and `std.optional.Some(...)`; the same implicit import may make
  `None` and `Some(...)` available in source.
- `Result` is a special standard import: the compiler behaves as if `imports result;` existed and brings `Result<T, E>`
  into scope. Most other `std.*` modules are not automatically placed in local scope.
- `Panic` is also an implicit standard import for assertion and panic macros. `assert!`, `assert_eq!`, `assert_ne!`,
  `debug_assert!`, `debug_assert_eq!`, and `panic!` are available without an explicit import, but they are still library
  macros rather than compiler built-ins.
- `Stdio` is not prelude and is not implicit. Its macros require `imports stdio;` or `using namespace stdio;`, except
  `format_args!`, which is built in.
- Legacy exception syntax is deprecated. Active examples should prefer `Result.Result<T, E>` and postfix `@` propagation;
  old `throws`/`throw`/`try`/`catch` spellings should appear only in explicitly marked legacy notes.

## Program examples

Each file in `Spec/Programs/` contains one example program. They are design fixtures for syntax, APIs, and semantic intent,
not compatibility promises for the current partial compiler implementation.

The current set intentionally covers different language surfaces:

- CLI/file workflows: `TodoCli.xs`, `FileBackup.xs`, `StaticSiteReport.xs`, `LogAggregator.xs`
- Data-heavy services: `BankLedger.xs`, `InventoryService.xs`, `CsvAnalytics.xs`
- Network and async sketches: `ChatServer.xs`, `HttpHealthMonitor.xs`, `SensorStream.xs`, `JobQueue.xs`
- Package/tooling flows: `PackageResolver.xs`, `PluginPipeline.xs`
