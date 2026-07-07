<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xs-backend

Future native XS Backend project.

This project is intentionally only a placeholder for now. When implementation
starts, it will be C23-first, may use isolated optional Rust where there is a
concrete technical reason, and may contain optional NASM `.asm`/`.inc` sources.

Planned layout:

- `sources/`: C23-first backend implementation.
- `include/`: public/internal backend headers.
- `asm/`: optional portable assembly support, without x86-only assumptions.
