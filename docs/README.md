<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs-project documentation

This directory contains the active architecture and implementation documentation for the X# compiler. The `Spec/` directory
is the source documentation area for syntax and language examples; `docs/` explains compiler architecture, the build/test
process, the CLI contract, and implementation status.

## Reading order

Recommended order for newcomers:

1. [../README.md](../README.md): repository overview and quick start
2. [BUILDING.md](BUILDING.md): build/test and toolchain
3. [ARCHITECTURE.md](ARCHITECTURE.md): compiler pipeline and layer boundaries
4. [CLI.md](CLI.md): user commands and current status
5. [IMPLEMENTATION.md](IMPLEMENTATION.md): stage-by-stage implementation status
6. [TODO.md](TODO.md): X# v0 decisions and remaining implementation work
7. [RELEASES.md](RELEASES.md): release policy tied to the LLVM IR milestone
8. [LLVM_BACKEND.md](LLVM_BACKEND.md): LLVM backend infrastructure
9. [MONOREPO.md](MONOREPO.md): monorepo project/runtime selection model

## Documentation authority

- For documented X# syntax, `Spec/` has priority.
- For the `.xsproj` format, `Spec/ProjectSystem.xs` and the public C API headers have priority.
- For XLIL decisions, read `docs/XLIL.md`, `docs/TODO.md`, and `xs/include/xs/lil.h` together.
- For implementation order, [IMPLEMENTATION.md](IMPLEMENTATION.md) is authoritative.
- Major semantic and ABI decisions are tracked as the X# v0 contract in [TODO.md](TODO.md).

If you find a conflict, do not silently add new behavior. Update the documentation and implementation in the same patch.

## Update rule

When code behavior changes, at least one document should change too:

- CLI changes go in `CLI.md`.
- Build/toolchain changes go in `BUILDING.md`.
- Pipeline or layer-boundary changes go in `ARCHITECTURE.md` and `IMPLEMENTATION.md`.
- Major language/ABI decisions go in `TODO.md`.
- User-visible changes are summarized in the root `CHANGELOG.md`.
