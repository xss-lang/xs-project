/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
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

pub use parser::parse_module;
pub use verify::verify_module;
pub use writer::{module_to_string, write_module};

#[cfg(test)]
mod float_operator_tests;
#[cfg(test)]
mod operator_tests;
#[cfg(test)]
mod string_comparison_tests;
#[cfg(test)]
mod unary_tests;
