<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Build and test guide

xs-project is built on C23, CMake, Ninja, Clang/LLVM, and LLD. The documented and tested build path uses the Clang/LLVM
toolchain.

## Required tools

- `cmake` 3.31 or newer
- `ninja`
- `clang`
- LLVM tools:
  - `llvm-ar`
  - `llvm-ranlib`
  - `llvm-nm`
  - `llvm-objcopy`
  - `llvm-objdump`
  - `llvm-strip`
- `ld.lld`
- `rustup` and `cargo`; the pinned `xslang/rust-toolchain.toml` toolchain must be installed

Useful helper tools:

- `fd`
- `rg`
- `bat -p`
- `sd`
- `busybox wc`
- `tokei`
- `uutils-coreutils` tools

## Default build

```text
cmake --preset clang-debug
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Preset details:

- generator: Ninja
- compiler: Clang
- build directory: `build/clang-debug`
- default project: `xs`

The `xs` target builds the Rust `xslang` static library into the CMake build directory and links its compiler-core session
API into the C23 driver. Building `xsproj` alone remains C23-only and does not require Rust or LLVM.

## OOM-safe workflow

Parser/project tests have previously triggered OOM conditions. Use a 2GB virtual memory limit for longer build/test runs:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

Tests are expected to run quickly. If a test suddenly consumes a lot of memory, treat it as a possible infinite loop, parser
progress bug, or runaway macro expansion.

## Sanitizer build

AddressSanitizer and UndefinedBehaviorSanitizer are available through the separate Clang preset:

```text
cmake --preset clang-sanitize
cmake --build --preset clang-sanitize
ctest --preset clang-sanitize --output-on-failure
```

Do not apply the 2GB virtual-memory limit to AddressSanitizer runs: its required shadow-memory reservation exceeds that
limit even when the program's real memory use is small.

## Project selection

Stable projects:

```text
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=xs
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=xsproj
cmake --preset clang-debug -DXS_ENABLE_PROJECTS=all
```

`xsproj` must not require the LLVM package when built by itself. Future projects (`xsfmt`, `xstidy`, and `xs-analyzer`)
intentionally produce CMake errors for now.

## Useful checks

```text
git diff --check
rg -n "\bNULL\b|#include <stdbool\.h>" xs xsproj tests include
busybox wc -l <file.c> <file.h>
```

Files must not exceed 1000 lines. New or touched C code should use `nullptr` and C23 `bool`.

## Build outputs

`build/` is a generated area. It may look dirty after build/test runs and should not be included in normal commits.
