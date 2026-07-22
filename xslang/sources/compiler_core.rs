/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod direct_ir;
mod model;
mod test_harness;
mod xlil_lowering;

pub mod hir_lowering;

pub use direct_ir::*;
pub use model::*;
