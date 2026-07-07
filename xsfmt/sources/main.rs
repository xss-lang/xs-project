/*
 * SPDX-FileCopyrightText: 2026 Leitwolf <xs-lang.chess031@slmails.com>
 * SPDX-License-Identifier: Apache-2.0
 */

use serde::{Deserialize, Serialize};

#[derive(Debug, Deserialize, Serialize)]
struct XsFmtConfig
{
  max_width: Option<u16>,
}

fn main()
{
  let _config = XsFmtConfig { max_width: None };
  eprintln!("xsfmt is registered in the XS monorepo, but implementation has not started yet.");
}
