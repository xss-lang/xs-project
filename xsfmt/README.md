<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: Apache-2.0
-->

# xsfmt

Future X# formatter project.

`xsfmt` is intentionally isolated from the C23 compiler implementation. It will
be implemented in Rust nightly and may use Serde for configuration/data models.

User configuration for `xsfmt` is standardized on TOML.

Rust sources live under `xsfmt/sources/`.
