/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod literal;
mod utf16;

pub(crate) use literal::{decode_character, format_character};
pub(crate) use utf16::{format_encoded, format_units, parse_encoded, parse_units};
