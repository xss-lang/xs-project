/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: MPL-2.0
 */

use serde::{Deserialize, Serialize};

#[derive(Debug, Deserialize, Serialize)]
struct XsTidyConfig
{
  deny_warnings: Option<bool>,
}

fn main()
{
  let _config = XsTidyConfig { deny_warnings: None };
  eprintln!("xstidy is registered in the XS monorepo, but implementation has not started yet.");
}
