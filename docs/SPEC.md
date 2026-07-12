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
- `Spec/Macros.xs`, `Spec/Stdio.xs`, and `Spec/Panic.xs` cover macro behavior and imported macro modules.
- `Spec/Data.xs`, `Spec/Enum.xs`, `Spec/Classes.xs`, and `Spec/Generics.xs` cover declaration forms.
- `Spec/FS.xs`, `Spec/Http.xs`, `Spec/Process.xs`, `Spec/Sync.xs`, and `Spec/Thread.xs` describe standard-library-facing
  examples.
- `Spec/Programs/` contains larger one-file programs written as if the completed language and standard library existed.

## Current example conventions

- Macro names are snake_case and are not under `STD`.
- Standard modules use `STD.<Module>` names, such as `STD.FS` and `STD.Collections`.
- `format_args!` and `include!` are built-in macros.
- `print!`, `println!`, `eprint!`, `eprintln!`, `write!`, `writeln!`, and `format!` come from `Stdio`.
- `Optional<T>` is available as if the compiler had inserted `imports Optional` and brought
  `STD.Optional.Optional<T>` into scope as `Optional<T>`.
- Optional value constructors are `STD.Optional.None` and `STD.Optional.Some(...)`; the same implicit import may make
  `None` and `Some(...)` available in source.
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
