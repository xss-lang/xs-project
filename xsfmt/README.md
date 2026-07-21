<!--
SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
SPDX-License-Identifier: MPL-2.0
-->

# xsfmt

Future X# formatter project.

`xsfmt` is intentionally isolated from the C23 compiler implementation. It will
be implemented in Rust nightly and may use Serde for configuration/data models.

User configuration for `xsfmt` is standardized on TOML.

The configuration model currently validates `max_width` (40–320),
`indent_width` (1–16), `use_tabs`, and `newline_style` (`auto`, `lf`, or
`cr_lf`). Formatting X# source is still a later milestone.

Rust sources live under `xsfmt/sources/`.
