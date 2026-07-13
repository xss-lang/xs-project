/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

mod model;
mod operations;
mod type_names;

pub use model::*;
pub use operations::*;
pub use type_names::*;

pub mod lowering;
pub mod parser;
pub mod verify;
pub mod writer;

#[cfg(test)]
mod operator_tests;
#[cfg(test)]
mod unary_tests;
