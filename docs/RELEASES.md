<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Release policy

xs-project uses a numbered pre-1.0 development line. The first line was `0.0.1`.

The `0.0.x` series is for compiler infrastructure milestones. It may include parsers, intermediate representations, public C
APIs, and backend checkpoints before the full native executable pipeline is complete.

## Current status

- Current project version: `0.1.3`.
- `xs --version` reports the configured compiler version.
- The root [../CHANGELOG.md](../CHANGELOG.md) file keeps `Unreleased` for work after the current numbered line.
- Numbered `0.0.x` entries are development snapshots, not stable language releases.

## Release metadata check

The root `release.java` source-file tool checks release metadata without using shell scripts:

```text
java --source=21 release.java check 0.1.3
```

It validates the CMake project version, changelog heading, release documentation, CLI version documentation, and the built
`xs --version` output when `build/clang-debug/xs` exists.

## Version line

- `0.1.3` adds Rust compiler-core loop CFG lowering and validated same-category multiple-inheritance graphs for nominal
  types.
- `0.1.2` connects supported compiler-core sessions back to native emission. Typed Rust HIR lowers through verified and
  optimized MIR into XLIL text, which the public C23 XLIL parser verifies before LLVM IR, object, and `.xse` emission.
- `0.1.1` establishes the live C23-to-Rust compiler-core boundary. Macro-expanded structural ASTs are imported into owned
  sessions and top-level function signatures begin lowering into Rust HIR while the 0.1.0 ELF baseline remains active.
- `0.1.0` is the first source-to-native Linux ELF milestone. The supported source subset lowers through MIR, XLIL, LLVM IR,
  ELF object emission, and LLD linking to a runnable `.xse` executable.
- `0.0.8` is the XLIL stack-memory snapshot: target-independent stack slots and typed load/store operations lower through
  LLVM `alloca`, `load`, and `store`, including direct XLIL native `.xse` builds.
- `0.0.7` is the source-native direct-call snapshot: checked source `main` can call same-module `Long -> Long` helpers and
  lower those calls through MIR, XLIL, LLVM IR, object emission, and native `.xse` linking.
- `0.0.6` is the CFFI surface snapshot: attributes and `extern "C"` blocks are parsed, collected into HIR symbols, and
  guarded by the first HIR C ABI validation pass.
- `0.0.5` is the source-native local-binding snapshot: checked source `main` can lower selected `Long`/`Bool` locals into
  the current native `.xse` slice.
- `0.0.4` is the first source-native control-flow snapshot: checked source `main` can lower selected i32 arithmetic and
  comparison `if` expressions to native `.xse`.
- `0.0.3` is the direct XLIL LLVM pipeline snapshot: verified/optimized LLVM emission plus i32 arithmetic lowering.
- Later `0.0.x` releases should keep landing narrow compiler infrastructure checkpoints without promising source-level
  completeness.
- The `0.1.x` line keeps the native pipeline operational while progressively replacing the narrow source-native bridge with
  the Rust typed-HIR/MIR compiler core.
- The first `.xse` target format is Linux ELF; PE comes after ELF support.

## 0.1.0 threshold

The expected minimum threshold for `0.1.0` is:

1. function body lowering from typed, borrow-checked MIR to XLIL,
2. XLIL to LLVM IR function body lowering,
3. LLVM module verification,
4. object emission,
5. linking according to project targets,
6. native artifact generation through `xs build`.

After this threshold is reached, user-visible changes may move from infrastructure snapshots toward a native compiler
release line.
