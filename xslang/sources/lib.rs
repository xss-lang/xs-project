/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#![warn(missing_docs)]

pub mod codegen;
pub mod compiler_core;
pub mod hir;
pub mod mir;
pub mod mono;
pub(crate) mod text;
pub mod xgc;
pub mod xlil;

/// Version of the `xslang` crate that built this compiler core.
pub const VERSION: &str = env!("XSLANG_BUILD_VERSION");
/// Highest XHIR text format version accepted by this release.
pub const XHIR_VERSION: &str = env!("XSLANG_XHIR_VERSION");
/// Highest XMIR text format version accepted by this release.
pub const XMIR_VERSION: &str = env!("XSLANG_XMIR_VERSION");
/// Highest XLIL text format version accepted by this release.
pub const XLIL_VERSION: &str = env!("XSLANG_XLIL_VERSION");
