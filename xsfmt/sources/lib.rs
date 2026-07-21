/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod config;
mod formatter;

pub use config::{Config, ConfigError, NewlineStyle};
pub use formatter::{FormatResult, format_source};
