<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# Contribution and workflow rules

This repository is compiler infrastructure. Prefer small, testable patches that preserve architecture boundaries.

## Core rules

- C23 is the primary implementation language.
- Rust should be used only in future tool projects (`xsfmt`, `xstidy`) or in isolated modules with a clear technical reason.
- Shell scripts are not added as persistent project artifacts.
- Use CMake 3.31 or newer; do not use Meson.
- Do not add dependencies on GNU toolchain behavior.
- Files must not exceed 1000 lines. New or touched C and header files should stay much smaller when practical.
- In new/touched C code, use `nullptr` instead of `NULL`, and C23 `bool` instead of `<stdbool.h>`.

## Patch size

Do not try to finish an entire compiler stage in one huge patch. Preferred shape:

1. add the data model
2. connect parser/HIR/MIR
3. add focused tests
4. update documentation
5. pass build/test

## Tests

Every change should at least pass the touched subsystem’s tests. Run the full test suite when practical:

```text
ulimit -v 2097152
cmake --build --preset clang-debug
ctest --preset clang-debug --output-on-failure
```

For docs-only changes, a build is not required, but `git diff --check` should be run.

## Documentation

If code behavior changes, the relevant documentation changes too. Every user-visible change is summarized in the root
`CHANGELOG.md`.

## Commits

Do not commit generated artifacts:

- `build/`
- `target/`
- `node_modules/`
- `dist/`
- `out/`

Preserve user changes; do not revert unrelated files.
