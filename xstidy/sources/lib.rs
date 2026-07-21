/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod config;
mod lint;
mod workspace;

pub use config::{Config, ConfigError};
pub use lint::{Diagnostic, Severity, lint_source};
pub use workspace::{FileDiagnostics, WorkspaceError, collect_xs_files, lint_paths};
