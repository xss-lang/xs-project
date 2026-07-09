<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Release policy

xs-project now uses a numbered pre-1.0 development line. The first line is `0.0.1`.

The `0.0.x` series is for compiler infrastructure milestones. It may include parsers, intermediate representations, public C
APIs, and backend checkpoints before the full native executable pipeline is complete.

## Current status

- Current project version: `0.0.1`.
- `xs --version` reports the configured compiler version.
- The root [../CHANGELOG.md](../CHANGELOG.md) file keeps `Unreleased` for work after the current numbered line.
- Numbered `0.0.x` entries are development snapshots, not stable language releases.

## Release metadata check

The root `release.java` source-file tool checks release metadata without using shell scripts:

```text
java --source=21 release.java check 0.0.1
```

It validates the CMake project version, changelog heading, release documentation, CLI version documentation, and the built
`xs --version` output when `build/clang-debug/xs` exists.

## Native executable threshold

The expected minimum threshold for a native-executable-oriented release remains:

1. function body lowering from typed, borrow-checked MIR to XLIL,
2. XLIL to LLVM IR function body lowering,
3. LLVM module verification,
4. object emission,
5. linking according to project targets,
6. native artifact generation through `xs build`.

After this threshold is reached, user-visible changes may move from infrastructure snapshots toward a native compiler
release line.
