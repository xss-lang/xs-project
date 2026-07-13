<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Release policy

xs-project uses a numbered pre-1.0 development line. The first line was `0.0.1`.

The `0.0.x` series is for compiler infrastructure milestones. It may include parsers, intermediate representations, public C
APIs, and backend checkpoints before the full native executable pipeline is complete.

## Current status

- Current project version: `0.0.7`.
- `xs --version` reports the configured compiler version.
- The root [../CHANGELOG.md](../CHANGELOG.md) file keeps `Unreleased` for work after the current numbered line.
- Numbered `0.0.x` entries are development snapshots, not stable language releases.

## Release metadata check

The root `release.java` source-file tool checks release metadata without using shell scripts:

```text
java --source=21 release.java check 0.0.7
```

It validates the CMake project version, changelog heading, release documentation, CLI version documentation, and the built
`xs --version` output when `build/clang-debug/xs` exists.

## Version line

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
- The `0.1.0` line starts when source/project `xs build` has a broader typed-MIR native path instead of the current narrow
  source-native slice.
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
