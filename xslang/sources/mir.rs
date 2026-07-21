/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

mod model;

pub use model::*;

pub mod optimizer;
pub mod text;
pub mod verify;

#[cfg(test)]
mod operator_optimizer_tests;
